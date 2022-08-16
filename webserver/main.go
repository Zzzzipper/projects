package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/signal"
	"regexp"
	"server/engine/gates"
	"strconv"
	"strings"
	"time"

	"server/engine"
	"server/engine/assets"
	"server/engine/bootstrap"
	"server/engine/cblocks"
	"server/engine/consts"
	"server/engine/domains"
	"server/engine/logger"
	"server/engine/modules"
	"server/engine/session"
	"server/engine/utils"
	"server/engine/workers"

	"github.com/vladimirok5959/golang-server-resources/resource"
	"github.com/vladimirok5959/golang-server-static/static"
	"github.com/vladimirok5959/golang-worker/worker"

	"gopkg.in/ini.v1"
)

var cliInitalized bool = false

func init() {

	var grpcAddress string

	flag.IntVar(&consts.ParamPort, "port", 8080, "TCP порт")
	flag.StringVar(&consts.ParamWwwDir, "dir", "", "Виртуальная папка для сценариев")
	flag.StringVar(&grpcAddress, "grpc", "", "TCP/IP адрес GRPC сервера")
	flag.IntVar(&consts.ParamGrpcGateTimeOut, "timeout", 30, "Время ожидания ответа от GRPC сервера (сек.)")
	flag.BoolVar(&consts.ParamDebug, "debug", false, "Отладочный режим с выводом в консоль")
	flag.BoolVar(&consts.ParamKeepAlive, "keepalive", false, "Разрешить/Запретить серверу закрывать сессии по истечении лимита")
	flag.Parse()

	// Пока так. Если эти параметры прочитаны, то значит запуск из командной строки
	if consts.ParamPort != 0 && len(consts.ParamWwwDir) > 0 {

		cliInitalized = true

		// Определяем GRPC сервер
		consts.HasGrpcGate = checkValidGrpcAddres(grpcAddress)

	}
}

//
// checkValidGrpcAddres - Парисинг адреса GRPC
//
func checkValidGrpcAddres(address string) bool {
	params := strings.Split(address, ":")
	if len(params) == 0 {
		fmt.Println("# Внимание! Адрес GRPC сервера не указан или запись имеет неверный формат..!")
	} else if len(params) == 2 {
		consts.ParamGrpcGateHost = params[0]
		if consts.ParamGrpcGateHost != "localhost" && !validIP4(consts.ParamGrpcGateHost) {
			fmt.Println("# Внимание! IP адрес GRPC сервера имеет неверный формат..!")
		} else {
			var errOfPortConv error
			consts.ParamGrpcGatePort, errOfPortConv = strconv.Atoi(params[1])
			if errOfPortConv != nil {
				fmt.Printf("# Внимание! При определении TCP порта GRPC сервера произошла ошибка..!\n")
			} else {
				return true
			}
		}
	}
	return false
}

//
// Валидация IP адреса
//
func validIP4(ipAddress string) bool {
	ipAddress = strings.Trim(ipAddress, " ")
	re, _ := regexp.Compile(`^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$`)
	if re.MatchString(ipAddress) {
		return true
	}
	return false
}

//
// Работа с ini файлом
//
func readOrCreateIni() bool {
	dir, errOfDir := os.Getwd()
	if errOfDir != nil {
		fmt.Printf("# Папка запуска не была определена!\n")
		dir = "."
	}

	cfg, err := ini.Load(dir + "/" + "WebServer.ini")
	if err != nil && cfg == nil {
		fmt.Printf("# Ошибка чтения *.ini файла: %v\n", err)

		// cfg = ini.Empty()

		// cfg.Section("server").Key("host").SetValue(consts.ParamHost)
		// cfg.Section("server").Key("port").SetValue(strconv.Itoa(consts.ParamPort))
		// cfg.Section("server").Key("dir").SetValue("./hosts")
		// cfg.SaveTo("server.ini")

		return false

	} else {
		fmt.Printf("# Чтение параметров запуска WEB сервера:\n")

		var errOfPortConv error

		/// Порт
		if cfg.Section("server").Key("port") != nil {
			consts.ParamPort, errOfPortConv = cfg.Section("server").Key("port").Int()
		}
		if errOfPortConv != nil {
			consts.ParamPort = 8088
			fmt.Printf("# Внимание! При определении TCP порта произошла ошибка, назначен %d..!\n", consts.ParamPort)
		}

		/// Папка dir
		if cfg.Section("server").Key("dir") != nil {
			consts.ParamWwwDir = cfg.Section("server").Key("dir").String()
		} else {
			consts.ParamWwwDir = "../Hosts"
		}
		if len(consts.ParamWwwDir) == 0 {
			fmt.Println("# Внимание! Виртуальная папка для сценария не указана, назначена ../Hosts..!")
		}

		/// Адрес GRPC сервера
		if cfg.Section("grpc").Key("address") != nil {
			consts.HasGrpcGate = checkValidGrpcAddres(cfg.Section("grpc").Key("address").String())
		}

		/// Таймаут ответа сервера
		if cfg.Section("grpc").Key("timeout") != nil {
			consts.ParamGrpcGateTimeOut, _ = cfg.Section("grpc").Key("timeout").Int()
		}

		// TODO: временно залочено, т.к. проверка смущает некоторых
		// кодеров своим наличием
		/*
			if cfg.Section("grpc").Key("empty_password") != nil {
				consts.AllowEmptyPassword, _ = cfg.Section("grpc").Key("empty_password").Bool()
			}
		*/

		fmt.Printf("# \n")
	}

	return true
}

func testConnection(host string, ports []string) (bool, error) {
	for _, port := range ports {
		conn, err := net.Listen("tcp", net.JoinHostPort(host, port))
		if err != nil {
			return false, err
		}
		if conn != nil {
			defer conn.Close()
		}
	}
	return true, nil
}

func main() {
	// Params from env vars
	read_env_params()

	fmt.Printf("# \n")
	fmt.Printf("# WEB сервер v.1.0.0.12\n")
	fmt.Printf("# \n")

	if !cliInitalized {
		// Проверка настроек по умолчанию
		readOrCreateIni()

		consts.ParamWwwDir = utils.FixPath(consts.ParamWwwDir)
		if len(consts.ParamWwwDir) == 0 {
			// папку не указали
			fmt.Printf("# Виртуальная папка для сценариев приложения не указана!\n")
			fmt.Printf("# Так же, параметры запуска можно будет указать в командной строке.\n")
			fmt.Printf("# Пример: ./(.\\)server(.exe) -port 8088 -dir ../Hosts -grpc localhost:50052\n")
			consts.ParamWwwDir = "../Hosts"
		}
	}

	fmt.Printf("# Параметры запуска: \n")
	fmt.Printf("# host:\t0.0.0.0\t- IP адрес прослушивания подключений\n")
	fmt.Printf("# port:\t%d\t- TCP порт прослушивания подключений\n", consts.ParamPort)
	fmt.Printf("# dir:\t%s\t- виртуальная папка для сценариев приложения\n", consts.ParamWwwDir)
	if consts.HasGrpcGate {
		fmt.Printf("# Есть параметры подключения к GRPC серверу:\n")
		fmt.Printf("# IP адрес: %s\n", consts.ParamGrpcGateHost)
		fmt.Printf("# Порт:     %d\n", consts.ParamGrpcGatePort)
	} else {
		fmt.Printf("# Параметры подключения к GRPC серверу не указаны.\n")
		fmt.Printf("# Сервер работает в режиме WEB-сервера.\n")
	}
	fmt.Printf("# \n")

	if !utils.IsDirExists(consts.ParamWwwDir) {
		fmt.Printf("# Виртуальную папку для сценариев не удается прочитать!\n")
		fmt.Printf("# Проверьте наличие папки в системе, дайте права доступа и сверьте с WebServer.ini,\n")
		fmt.Printf("# либо укажите проверенные параметры в командной строке.\n")
		fmt.Printf("# Остановите сервер (Ctrl+C)..\n")
		fmt.Printf("# \n")
		c := make(chan os.Signal, 1)
		signal.Notify(c, os.Interrupt)
		<-c
		os.Exit(-1)
	}

	var ports []string
	ports = make([]string, 1)
	ports[0] = strconv.Itoa(consts.ParamPort)
	_, err := testConnection("0.0.0.0", ports)
	if err != nil {
		fmt.Printf("# Внимание, порт %d для прослушивания не доступен, ошибка:\n\t\t%s\n", consts.ParamPort, err.Error())
		fmt.Printf("# Проверьте, не запущен ли еще один сервер на нем?\n")
		fmt.Printf("# Остановите сервер (Ctrl+C)..\n")
		fmt.Printf("# \n")
		c := make(chan os.Signal, 1)
		signal.Notify(c, os.Interrupt)
		<-c
		os.Exit(-1)
	}

	// Init logger
	logs := logger.New()

	// Attach www dir to logger
	logs.SetWwwDir(consts.ParamWwwDir)

	// Session cleaner
	wSessCl := workers.SessionCleaner(consts.ParamWwwDir)

	// Image processing
	wImageGen := workers.ImageGenerator(consts.ParamWwwDir)

	// Init mounted resources
	res := resource.New()
	assets.PopulateResources(res)

	// Init static files helper
	stat := static.New(consts.DirIndexFile)

	// Init modules
	mods := modules.New()

	// Init cache blocks
	cbs := cblocks.New()

	// Init and start web server
	server_address := fmt.Sprintf("%s:%d", "0.0.0.0", consts.ParamPort)

	// Server params
	server_params := func(s *http.Server) {
		s.SetKeepAlivesEnabled(consts.ParamKeepAlive)
	}

	// Create SCADA gates pool
	gatespool := gates.New()

	// Before callback
	before := func(
		ctx context.Context,
		w http.ResponseWriter,
		r *http.Request,
		o *[]bootstrap.Iface,
	) {
		w.Header().Set("Server", "kotmi-web.pro/"+consts.ServerVersion)
	}

	// After callback
	after := func(
		ctx context.Context,
		w http.ResponseWriter,
		r *http.Request,
		o *[]bootstrap.Iface,
	) {
		// Schema
		r.URL.Scheme = "http"

		// Convert
		var logs *logger.Logger
		if v, ok := (*o)[0].(*logger.Logger); ok {
			logs = v
		}

		var gatespool *gates.Pool
		if v, ok := (*o)[6].(*gates.Pool); ok {
			gatespool = v
		}

		var res *resource.Resource
		if v, ok := (*o)[3].(*resource.Resource); ok {
			res = v
		}

		var mods *modules.Modules
		if v, ok := (*o)[5].(*modules.Modules); ok {
			mods = v
		}

		// Mounted assets
		if res.Response(
			w,
			r,
			func(
				w http.ResponseWriter,
				r *http.Request,
				i *resource.OneResource,
			) {
				if consts.ParamDebug && i.Path == "assets/cp/scripts.js" {
					w.Write([]byte("window.debug=true;"))
				}
			},
			nil,
		) {
			return
		}

		// Host and port
		host, port := utils.ExtractHostPort(r.Host, false)
		curr_host := host

		// Domain bindings
		doms := domains.New(consts.ParamWwwDir)

		if mhost := doms.GetHost(host); mhost != "" {
			curr_host = mhost
		}

		vhost_dir := consts.ParamWwwDir + string(os.PathSeparator) + curr_host

		if !utils.IsDirExists(vhost_dir) {
			curr_host = "localhost"
			vhost_dir = consts.ParamWwwDir + string(os.PathSeparator) + "localhost"
		}

		vhost_dir_htdocs := vhost_dir + string(os.PathSeparator) + "htdocs"
		vhost_dir_logs := vhost_dir + string(os.PathSeparator) + "logs"
		vhost_dir_tmp := vhost_dir + string(os.PathSeparator) + "tmp"

		if !utils.IsDirExists(vhost_dir_htdocs) {
			utils.SystemErrorPageEngine(
				w,
				errors.New("Folder "+vhost_dir_htdocs+" is not found"),
			)
			return
		}
		if !utils.IsDirExists(vhost_dir_logs) {
			utils.SystemErrorPageEngine(
				w,
				errors.New("Folder "+vhost_dir_logs+" is not found"),
			)
			return
		}

		if !utils.IsDirExists(vhost_dir_tmp) {
			utils.SystemErrorPageEngine(
				w,
				errors.New("Folder "+vhost_dir_tmp+" is not found"),
			)
			return
		}

		// Session
		sess := session.New(w, r, vhost_dir_tmp)
		defer sess.Close()

		if engine.Response(
			gatespool,
			logs,
			mods,
			w,
			r,
			sess,
			cbs,
			host,
			port,
			curr_host,
			vhost_dir_htdocs,
			vhost_dir_logs,
			vhost_dir_tmp,
		) {
			return
		}

		// Error 404
		utils.SystemErrorPage404(w)
	}

	// Shutdown callback
	shutdown := func(
		ctx context.Context,
		o *[]bootstrap.Iface,
	) error {
		var errs []string

		if wImageGen, ok := (*o)[2].(*worker.Worker); ok {
			if err := wImageGen.Shutdown(ctx); err != nil {
				errs = append(errs, fmt.Sprintf("(%T): %s", wImageGen, err.Error()))
			}
		}

		if wSessCl, ok := (*o)[1].(*worker.Worker); ok {
			if err := wSessCl.Shutdown(ctx); err != nil {
				errs = append(errs, fmt.Sprintf("(%T): %s", wSessCl, err.Error()))
			}
		}

		if gatespool, ok := (*o)[6].(*gates.Pool); ok {
			if err := gatespool.Close(); err != nil {
				errs = append(errs, fmt.Sprintf("(%T): %s", gatespool, err.Error()))
			}
		}

		if logs, ok := (*o)[0].(*logger.Logger); ok {
			logs.Close()
		}

		if len(errs) > 0 {
			return errors.New("Shutdown callback: " + strings.Join(errs, ", "))
		}

		return nil
	}

	// Start server
	bootstrap.Start(
		&bootstrap.Opts{
			Handle:   logs.Handler,
			Host:     server_address,
			Path:     consts.AssetsPath,
			Cbserv:   server_params,
			Before:   before,
			After:    after,
			Timeout:  8 * time.Second,
			Shutdown: shutdown,
			Objects: &[]bootstrap.Iface{
				logs,
				wSessCl,
				wImageGen,
				res,
				stat,
				mods,
				gatespool,
			},
		},
	)
}

func read_env_params() {
	// TODO: safe this for feature
	// if consts.ParamHost == "0.0.0.0" {
	// 	if os.Getenv("KOTMI_WEB_HOST") != "" {
	// 		consts.ParamHost = os.Getenv("KOTMI_WEB_HOST")
	// 	}
	// }
	if consts.ParamPort == 8080 {
		if os.Getenv("KOTMI_WEB_PORT") != "" {
			consts.ParamPort = utils.StrToInt(os.Getenv("KOTMI_WEB_PORT"))
		}
	}
	if consts.ParamWwwDir == "" {
		if os.Getenv("KOTMI_WEB_DIR") != "" {
			consts.ParamWwwDir = os.Getenv("KOTMI_WEB_DIR")
		}
	}
	if consts.ParamDebug == false {
		if os.Getenv("KOTMI_WEB_DEBUG") == "true" {
			consts.ParamDebug = true
		}
	}
	if consts.ParamKeepAlive == false {
		if os.Getenv("KOTMI_WEB_KEEPALIVE") == "true" {
			consts.ParamKeepAlive = true
		}
	}
}

func ServeTemplateFile(
	w http.ResponseWriter,
	r *http.Request,
	file string,
	path string,
	dir string,
) bool {
	if r.URL.Path == "/"+path+file {
		if utils.IsRegularFileExists(dir + string(os.PathSeparator) + file) {
			http.ServeFile(w, r, dir+string(os.PathSeparator)+file)
			return true
		}
	}
	return false
}
