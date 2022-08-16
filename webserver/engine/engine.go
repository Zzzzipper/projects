package engine

import (
	"errors"
	"fmt"
	"net/http"
	"server/engine/cblocks"
	"server/engine/consts"
	"server/engine/gates"
	"server/engine/logger"
	"server/engine/modules"
	"server/engine/session"
	"server/engine/utils"
	"server/engine/wrapper"

	"github.com/vladimirok5959/golang-server-static/static"
)

type Engine struct {
	Wrap *wrapper.Wrapper
	Mods *modules.Modules
}

func Response(gates *gates.Pool, l *logger.Logger, m *modules.Modules, w http.ResponseWriter, r *http.Request, s *session.Session, c *cblocks.CacheBlocks, host, port, chost, dirHtdocs, dirLogs, dirTmp string) bool {
	wrap := wrapper.New(l, w, r, s, c, host, port, chost, dirHtdocs, dirLogs, dirTmp, gates)
	eng := &Engine{
		Wrap: wrap,
		Mods: m,
	}
	return eng.Process()
}

func (this *Engine) Process() bool {
	// Request was canceled
	if this.contextDone() {
		return false
	}

	// config := config.ConfigNew()
	// config.ConfigRead(this.Wrap.CurrHost + string(os.PathSeparator) + "config.json")

	// this.Wrap.IsBackend = this.Wrap.R.URL.Path == "/cp" || strings.HasPrefix(this.Wrap.R.URL.Path, "/cp/")
	// this.Wrap.ConfMysqlExists = utils.IsConfigExists(this.Wrap.DConfig + string(os.PathSeparator) + config.DbConection.CurrDriver + ".json")
	this.Wrap.UrlArgs = append(this.Wrap.UrlArgs, utils.UrlToArray(this.Wrap.R.URL.Path)...)

	fmt.Println("File: ", this.Wrap.R.URL.Path, ", Args: ", this.Wrap.UrlArgs, ", Body: ", this.Wrap.R.Body)

	// if this.Wrap.IsBackend && len(this.Wrap.UrlArgs) >= 1 && this.Wrap.UrlArgs[0] == "cp" {
	// 	this.Wrap.UrlArgs = this.Wrap.UrlArgs[1:]
	// }

	// Action
	if this.Mods.XXXActionFire(this.Wrap) {
		return true
	}

	// Request was canceled
	if this.contextDone() {
		return false
	}

	// Сразу делаем пользователя. Это просто его номер. Внутренний номер сессии.

	fmt.Println("user_id: ", this.Wrap.S.GetInt("UserId", -1))

	user_id := this.Wrap.S.GetInt("UserId", -1)
	if user_id < 0 {
		this.Wrap.S.SetInt("UserId", int32(utils.GetUID(8)))
	}

	// TODO: часть эксперимента - авторизация перед обработкой
	// POST запроса должна быть!
	// upd. Комментим, так как POST авторизация от Vue фронтенда
	// сама инитит сессию
	/*
		if this.Wrap.S.GetInt("UserId", 0) <= 0 {
			// Redirect
			if this.redirectFixCpUrl() {
				return true
			}
			// Show login form
			utils.SystemRenderTemplate(this.Wrap.W, assets.TmplLogin, nil, "", "")
			return true
		}
	*/

	// Redirect to CP for creating MySQL config file
	// if !this.Wrap.IsBackend && !this.Wrap.ConfMysqlExists {
	// 	this.Wrap.W.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
	// 	http.Redirect(this.Wrap.W, this.Wrap.R, this.Wrap.R.URL.Scheme+"://"+this.Wrap.R.Host+"/cp/", 302)
	// 	return true
	// }

	// Display MySQL install page on backend
	// if this.Wrap.IsBackend && !this.Wrap.ConfMysqlExists {
	// 	// Redirect
	// 	if this.redirectFixCpUrl() {
	// 		return true
	// 	}
	// 	switch config.DbConection.CurrDriver {
	// 	case "mysql":
	// 		// Show mysql settings form
	// 		utils.SystemRenderTemplate(this.Wrap.W, assets.TmplCpMySql, nil, "", "")
	// 		return true
	// 	case "sqlite3":
	// 		fmt.Println("Run sqlite3 database")
	// 		break
	// 	}
	// }

	// Request was canceled
	// if this.contextDone() {
	// 	return false
	// }

	// Check for MySQL connection
	// err := this.Wrap.UseDatabase()
	// if err != nil {
	// 	utils.SystemErrorPageEngine(this.Wrap.W, err)
	// 	return true
	// }

	if consts.HasGrpcGate {
		// TODO: создание подключений к шлюзам происходит
		// в обработчике формы авторизации
		// Check for Gates connections
		var result *gates.Result
		result = this.Wrap.UseGatesConnections()
		fmt.Println("- " + result.Message())
		if result.HasError() {
			this.Wrap.LogError(result.Message())
			utils.SystemErrorPageEngine(this.Wrap.W, errors.New(result.Message()))
			return true
		}

		// Request was canceled
		if this.contextDone() {
			return false
		}
	}

	// Запрос данных через POST
	if this.Mods.XXXRequestDataFire(this.Wrap) {
		return true
	}

	// Request was canceled
	if this.contextDone() {
		return false
	}

	// TODO: работаем со статикой - перенесено из main.go
	stat := static.New(consts.DirIndexFile)
	if stat.Response(this.Wrap.DHtdocs, this.Wrap.W, this.Wrap.R, nil, nil) {
		return true
	}

	// Separated logic
	// if !this.Wrap.IsBackend {
	// 	// Maintenance mode
	// 	if this.Wrap.Config.Engine.Maintenance != 0 {
	// 		if this.Wrap.User == nil {
	// 			// this.Wrap.UseDatabase()
	// 			this.Wrap.LoadSessionUser()
	// 		}
	// 		if this.Wrap.User == nil {
	// 			this.Wrap.RenderFrontEnd("maintenance", nil, http.StatusServiceUnavailable)
	// 			return true
	// 		}
	// 		if this.Wrap.User.A_id <= 0 {
	// 			this.Wrap.RenderFrontEnd("maintenance", nil, http.StatusServiceUnavailable)
	// 			return true
	// 		}
	// 		if this.Wrap.User.A_admin <= 0 {
	// 			this.Wrap.RenderFrontEnd("maintenance", nil, http.StatusServiceUnavailable)
	// 			return true
	// 		}
	// 	}

	// 	// Check User credentials
	// 	// Show login page if need
	// 	if this.Wrap.S.GetInt("UserId", 0) <= 0 {
	// 		// Redirect
	// 		if this.redirectFixCpUrl() {
	// 			return true
	// 		}
	// 		// Show login form
	// 		utils.SystemRenderTemplate(this.Wrap.W, assets.TmplLogin, nil, "", "")
	// 		return true
	// 	}

	// 	// Try load current user data
	// 	if !this.Wrap.LoadSessionUser() {
	// 		http.Redirect(this.Wrap.W, this.Wrap.R, "/", 302)
	// 		return true
	// 	}

	// 	// Render frontend
	// 	return this.Mods.XXXFrontEnd(this.Wrap)
	// }

	// Request was canceled
	if this.contextDone() {
		return false
	}

	// Show login page if need
	// if this.Wrap.S.GetInt("UserId", 0) <= 0 {
	// 	// Redirect
	// 	if this.redirectFixCpUrl() {
	// 		return true
	// 	}
	// 	// Show login form
	// 	utils.SystemRenderTemplate(this.Wrap.W, assets.TmplCpLogin, nil, "", "")
	// 	return true
	// }

	// Request was canceled
	// if this.contextDone() {
	// 	return false
	// }

	//
	// Здесь можно заюзать внутренний html
	//

	// Try load current user data
	if !this.Wrap.LoadSessionUser() {
		http.Redirect(this.Wrap.W, this.Wrap.R, "/", 302)
		return true
	}

	// Request was canceled
	if this.contextDone() {
		return false
	}

	// Only active admins can use backend
	// if !(this.Wrap.User.A_admin == 1 && this.Wrap.User.A_active == 1) {
	// 	// Redirect
	// 	if this.redirectFixCpUrl() {
	// 		return true
	// 	}
	// 	// Show login form
	// 	utils.SystemRenderTemplate(this.Wrap.W, assets.TmplCpLogin, nil, "", "")
	// 	return true
	// }

	// Redirect
	// if this.redirectFixCpUrl() {
	// 	return true
	// }

	// Request was canceled
	// if this.contextDone() {
	// 	return false
	// }

	// Render backend
	// return this.Mods.XXXBackEnd(this.Wrap)
	return true
}

// func (this *Engine) redirectFixCpUrl() bool {
// 	if len(this.Wrap.R.URL.Path) > 0 && this.Wrap.R.URL.Path[len(this.Wrap.R.URL.Path)-1] != '/' {
// 		http.Redirect(this.Wrap.W, this.Wrap.R, this.Wrap.R.URL.Path+"/"+utils.ExtractGetParams(this.Wrap.R.RequestURI), 302)
// 		return true
// 	}
// 	return false
// }

func (this *Engine) contextDone() bool {
	select {
	case <-this.Wrap.R.Context().Done():
		return true
	default:
	}
	return false
}
