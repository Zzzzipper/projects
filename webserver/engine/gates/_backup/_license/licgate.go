package license

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"server/engine/config"
	"server/engine/consts"
	"server/engine/gates"
	"server/engine/gates/license/licgate"
	"server/engine/gates/utils"
	"strconv"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"
)

// Gate - контейнер gRPC шлюза для Scada сервера
type gate struct {
	name    string                       // Имя канала, по которому из конфигурации берется разделс с настройками gRPC соединения
	user_id int32                        // Индекс пользователя
	token   gates.Token                  // Данные сессии
	client  licgate.LicenseServiceClient // gRPC реализация сервиса License
	conn    *grpc.ClientConn             // Указатель на объект gRPC соединения

	gates.IGate
}

func NewGate(name_ string, user_id_ int32) *gate {
	return &gate{
		name:    name_,
		user_id: user_id_,
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
func (this *gate) checkValidResponse(r *licgate.Response, e error) (string, error) {

	close := func(gt *gate) {
		fmt.Println("Start gate disconnect..")
		for g := gates.New(); g != nil; g = nil {
			fmt.Printf("Delete %s:%d gate from pool\n", this.name, this.user_id)
			g.Del(this.name, this.user_id)
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
		time.Second*time.Duration((*conf).Grpc.Gates["licgate"].QueryTimeOut))
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

	// Если соединяемся снова - отключаем сессию сервиса, если она была и обнуляем
	// указатель на нее

	if this.client != nil {
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
		utils.Log("[License] GRPC connection", time.Now(), err, true)
		this.conn = nil
		return err
	}

	// Инициализация gRPC драйвера и proto интерфейсов
	client := licgate.NewLicenseServiceClient(conn)
	log.Println("контекст получен..")

	// go say("Работает!")

	utils.Log("[License] OPEN", time.Now(), fmt.Errorf("success"), true)

	this.client = client
	this.conn = conn

	fmt.Printf("licgate entire open: %v\n", *this)

	return nil

}

//
// Нужна ли авторизация для шлюза?
//
func AuthRequired() bool {
	return false
}

//
// Close - закрыть соединение
//
func (this *gate) Close() error {

	fmt.Println("Start licgate Close ..")

	if consts.ParamDebug {
		utils.Log("[License] CLOSE", time.Now(), errors.New("success"), true)
	}

	if this.client != nil {
		this.client = nil
	}

	if this.conn != nil {
		this.conn.Close()
		this.conn = nil
	}

	return nil
}

func (this *gate) checkConnection() error {
	if this.conn == nil {
		return this.Connect()
	}
	return nil
}

//
// DoRequest
//
func (this *gate) DoRequest(command string, message string) (string, error) {

	// Проверка и попытка устновить соединение
	err := this.checkConnection()
	if err != nil {
		return "", err
	}

	id := this.token.Ses_id
	if command != "create_module" {
		id = int32(this.token.Mod_id)
	}

	ctx, cancel := this.prepareContext(map[string]string{
		"token":  utils.Int32ToString(this.token.Token),
		"mod_id": utils.Int32ToString(int32(this.token.Mod_id)),
	})

	defer cancel()

	r, err := this.client.DoRequest(ctx, &licgate.Request{ObjId: id,
		Command: command, Message: []byte(message)})
	if err != nil {
		return "", err
	}

	module := make(map[string]interface{})
	err = json.Unmarshal(r.Message, &module)

	if err == nil {
		m, ok := module["mod_id"].(float64)
		if ok {
			this.token.Mod_id = m
		}
	}

	return string(r.Message), nil
}
