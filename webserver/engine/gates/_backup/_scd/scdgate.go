package scd

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"server/engine/config"
	"server/engine/consts"
	"server/engine/gates"
	"server/engine/gates/scd/scdgate"
	"server/engine/gates/utils"
	"strconv"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
)

// Gate - контейнер gRPC шлюза для Scada сервера
type gate struct {
	address  string                   // IP адрес сервера приложений
	login    string                   // Логин сервера приложений (зашифрованый)
	password string                   // Пароль сервера приложений (MD5)
	port     uint                     // TCP Порт
	name     string                   // Имя канала, по которому из конфигурации берется разделс с настройками gRPC соединения
	client   scdgate.MdxServiceClient // gRPC реализация сервиса Mdx
	conn     *grpc.ClientConn         // Указатель на объект gRPC соединения
	obj_id   int32                    // Индекс MDX соединения
	token    gates.Token              // Параметры сессии

	gates.IGate
}

func NewGate(address_, login_, password_ string, port_ uint, name_ string) *gate {
	return &gate{
		address:  address_,
		login:    login_,
		password: password_,
		port:     port_,
		name:     name_,
	}
}

//
// say - тестовая рутина для контроля соединения
//
func say(s string) {
	for {
		time.Sleep(1000 * time.Millisecond)
		fmt.Println(s)
	}
}

//
// MakeErrorJson - создает JSON строку
//
func makeJsonError(message string) string {
	errormap := map[string]string{"error": message}
	jsonE, _ := json.Marshal(errormap)
	return string(jsonE)
}

//
// checktValidResponse - возвращает результат, упакованный сервером с типом ответа
// и управляет подключениями, если возникают проблемы
//
func (this *gate) checkValidResponse(r *scdgate.Response, e error) (string, error) {

	close := func(gt *gate) {
		fmt.Println("Start gate disconnect..")
		for g := gates.New(); g != nil; g = nil {
			fmt.Printf("Delete %s:%d gate from pool\n", this.name, this.token.Ses_id)
			g.Del(this.name, this.token.Ses_id)
		}
		gt.Close()
	}

	if e != nil {
		defer close(this)
		return "", e
	} else {
		text := string(r.GetMessage())
		if len(text) > 0 {
			if strings.Contains(text, "success") == true {
				return strings.Replace(text, "success:", "", 1), nil
			} else if strings.Contains(text, "failed") == true {
				defer close(this)
				return "", errors.New(strings.Replace(text, "failed:", "", 1))
			}
		}
	}

	return "", errors.New("Empty responce...")
}

//
// prepareContext -запись в конекст пары ключ:значение
//
func (this *gate) prepareContext(meta map[string]string) (context.Context, context.CancelFunc) {
	//fmt.Println("Pack token = ", token)
	//fmt.Println("Pack token = ", this.token.Token, ", Token: ", this.token)
	conf := config.ConfigNew()
	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration((*conf).Grpc.Gates["scdgate"].QueryTimeOut))
	header := metadata.New(meta)
	// this is the critical step that includes your headers
	return metadata.NewOutgoingContext(ctx, header), cancel
}

// Connect - Подключиться к серверу, инициализировать указатели соединения. При первой попытке
// соединения серверу передается уникальный fingerprint клиента для формирвания Jwt
// токена
func (this *gate) Connect() error {
	// TODO: здесь критично время чтения, т.к. читаться должно после начала сессии
	conf := config.ConfigNew()

	// Создание контекста соединения: запись fingerprint
	// fingerprint формируется как AES hash из логина и пароля

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration((*conf).Grpc.Gates[this.name].QueryTimeOut))
	defer cancel()

	// Если соединяемся снова - отключаем сессию сервиса, если она была и обнуляем
	// указатель на нее

	if this.client != nil {
		this.client.DoClose(ctx, &scdgate.Logout{ObjId: this.obj_id})
		this.client = nil
	}

	// Подключение собственно самого сервера и создание соединения
	conn, err := grpc.Dial(
		(*conf).Grpc.Gates[this.name].ServerAddress+":"+strconv.Itoa((*conf).Grpc.Gates[this.name].ServerPort),
		grpc.WithInsecure(), grpc.WithBlock(),
		grpc.WithTimeout(time.Second*time.Duration((*conf).Grpc.Gates[this.name].DialServerTimeOut)),
	)

	if err != nil {
		log.Printf("Нет соединения c GRPC сервером: %v", err)
		utils.Log("[Scd] GRPC connection", time.Now(), err, true)
		this.conn = nil
		return err
	}

	// Инициализация gRPC драйвера и proto интерфейсов
	client := scdgate.NewMdxServiceClient(conn)
	log.Println("контекст получен..")

	// go say("Работает!")

	// Создание контекста соединения: запись fingerprint
	// fingerprint формируется как AES hash из логина и пароля
	// hash := aes.Encrypt([]byte(login+password), login+password)
	// ctx, cancel := prepareContext(map[string]string{"fingerprint": string(hash)})

	// ctx, cancel := context.WithTimeout(context.Background(),
	// 	time.Second*time.Duration((*conf).Grpc.Gates[this.name].QueryTimeOut))
	// defer cancel()

	// TODO: внимательно, здесь подмена интерфейсов
	// r, errOpen := client.Open(ctx, &scdgate.OpenRequest{Address: address,
	// 	Login: login, Password: password, Port: uint32(port)})

	// r, errOpen := client.OpenEncrypted(ctx, &scdgate.OpenRequestEncrypted{Address: address,
	// 	Port: uint32(port), Hash: hash})

	fmt.Printf("Пытаемся подключить %s:%s\n", this.login, this.password)

	r, errOpen := client.DoOpen(ctx, &scdgate.Login{Login: this.login, Password: this.password})

	if errOpen != nil {
		fmt.Println(fmt.Sprintf("в подключении %s:%s:%s:%d к SCADA серверу отказано: %v",
			this.address, this.login, this.password, this.port, errOpen))
		if grpc.Code(err) == codes.Unavailable {
			this.token = gates.Token{0, 0, 0}
		}
		if consts.ParamDebug {
			utils.Log("[Scd] OPEN", time.Now(), err, true)
		}
		this.client = nil
		return errOpen
	} else {
		m := string(r.GetMessage())
		// if len(m) > 0 && strings.Contains(m, "success") == true {
		// 	fmt.Println("Received: ", m)
		// 	r := strings.Split(m, ":")
		// 	if len(r) > 0 {
		// 		token = r[1]
		// 	}
		// } else {
		// 	return nil, err
		// }
		fmt.Println("Success received: ", m)
		err := json.Unmarshal(r.GetMessage(), &this.token)
		if err != nil {
			return err
		}
	}

	errSuccess := fmt.Errorf("success")
	if consts.ParamDebug {
		utils.Log("[Scd] OPEN", time.Now(), errSuccess, true)
	}

	this.client = client
	this.conn = conn

	fmt.Printf("scdgate entire open: %v\n", *this)

	return nil

}

func (this *gate) Login() string {
	return this.login
}

func (this *gate) Password() string {
	return this.password
}

func (this *gate) SessionId() int32 {
	return this.token.Ses_id
}

//
// Нужна ли авторизация для шлюза?
//
func AuthRequired() bool {
	return true
}

//
// Close - закрыть соединение
//
func (this *gate) Close() error {
	fmt.Println("Start scdgate Close ..")

	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
	defer cancel()

	err := errors.New("success")
	if consts.ParamDebug {
		utils.Log("[Scd] CLOSE", time.Now(), err, true)
	}

	if this.client != nil {
		this.client.DoClose(ctx, &scdgate.Logout{ObjId: this.obj_id})
		this.client = nil
	}

	if this.conn != nil {
		this.conn.Close()
		this.conn = nil
	}

	return err
}

func (this *gate) checkConnection() error {
	if this.conn == nil {
		return this.Connect()
	}
	return nil
}

//
// Menu - получить меню в JSON
//
// func (this *gate) Menu() (string, error) {
// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}
// 	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
// 	defer cancel()
// 	r, err := this.client.Menu2Json(ctx, &empty.Empty{})

// 	return this.checkValidResponse(r, err)
// }

//
// Infrastructure - получить дерево объектов инфраструктуры
//
// func (this *gate) Infrastructure() (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
// 	defer cancel()
// 	r, err := this.client.Objects2Js(ctx, &empty.Empty{})

// 	return this.checkValidResponse(r, err)

// }

//
// Retro - получить файл формы ретроспективы
//
// func (this *gate) Retro(file string) (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
// 	defer cancel()
// 	r, err := this.client.Retro2Xml(ctx, &scdgate.RetroRequest{Fname: file})
// 	outbuf, err := this.checkValidResponse(r, err)
// 	if err != nil {
// 		return "", err
// 	}

// 	xml := strings.NewReader(outbuf)

// 	json, err := xj.Convert(xml)
// 	if err != nil {
// 		panic("That's embarrassing...")
// 	}

// 	return json.String(), nil
// }

//
// Retro - получить данные ретроспективы по индексу измерения во временном
// диапазоне
//
// func (this *gate) Measuring(id []string, startTime int64, endTime int64) (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
// 	defer cancel()
// 	r, err := this.client.Measuring2Xml(ctx, &scdgate.MeasuringRequest{Id: id, StartTime: startTime, EndTime: endTime})
// 	outbuf, err := this.checkValidResponse(r, err)
// 	if err != nil {
// 		return "", err
// 	}

// 	xml := strings.NewReader(outbuf)

// 	json, err := xj.Convert(xml)
// 	if err != nil {
// 		panic("That's embarrassing...")
// 	}

// 	return json.String(), nil
// }

//
// LockIds - получение Lock ID измерения по уникальному ключу
//
// func (this *gate) LockIds(ids []string) (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	ctx, cancel := this.prepareContext(map[string]string{"token": utils.Int32ToString(this.token.Token)})
// 	defer cancel()
// 	r, err := this.client.LocIds(ctx, &scdgate.MeasuringRequest{Id: ids})
// 	outbuf, err := this.checkValidResponse(r, err)
// 	if err != nil {
// 		return "", err
// 	}

// 	xml := strings.NewReader(outbuf)

// 	json, err := xj.Convert(xml)
// 	if err != nil {
// 		panic("That's embarrassing...")
// 	}

// 	return json.String(), nil
// }

//
// Ping
//
// func (this *gate) Ping() (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	id := this.token.Ses_id

// 	ctx, cancel := this.prepareContext(map[string]string{
// 		"token":  utils.Int32ToString(this.token.Token),
// 		"ses_id": utils.Int32ToString(id),
// 		"mod_id": utils.Int32ToString(int32(this.token.Mod_id)),
// 	})

// 	defer cancel()

// 	r, err := this.client.Ping(ctx, &scdgate.Request{
// 		ObjId:   id,
// 		Command: "ping",
// 		Message: []byte(""),
// 	})

// 	if err != nil {
// 		return "", err
// 	}

// 	return string(r.Message), nil
// }

//
// DoRequest
//
// func (this *gate) DoRequest(command string, message string) (string, error) {

// 	// Проверка и попытка устновить соединение
// 	err := this.checkConnection()
// 	if err != nil {
// 		return "", err
// 	}

// 	id := this.token.Ses_id
// 	if command != "create_module" {
// 		id = int32(this.token.Mod_id)
// 	}

// 	ctx, cancel := this.prepareContext(map[string]string{
// 		"token":  utils.Int32ToString(this.token.Token),
// 		"mod_id": utils.Int32ToString(int32(this.token.Mod_id)),
// 	})

// 	defer cancel()

// 	r, err := this.client.DoRequest(ctx, &scdgate.Request{ObjId: id,
// 		Command: command, Message: []byte(message)})
// 	if err != nil {
// 		return "", err
// 	}

// 	module := make(map[string]interface{})
// 	err = json.Unmarshal(r.Message, &module)

// 	if err == nil {
// 		m, ok := module["mod_id"].(float64)
// 		if ok {
// 			this.token.Mod_id = m
// 		}
// 	}

// 	return string(r.Message), nil
// }
