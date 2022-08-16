package grpc

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"server/engine/consts"
	"server/engine/gates"
	"server/engine/gates/grpc/grpcgate"
	"server/engine/gates/utils"
	"strconv"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/status"
)

// Gate - контейнер gRPC шлюза для Scada сервера
type gate struct {
	address string                     // IP адрес сервера приложений
	port    uint                       // TCP Порт
	client  grpcgate.GrpcServiceClient // gRPC реализация сервиса Mdx
	conn    *grpc.ClientConn           // Указатель на объект gRPC соединения
	ses_id  int32                      // Индекс MDX соединения
	user_id int32                      // Идентификатор пользователя, для которого создано соединение

}

func NewGate(address_ string, port_ uint) *gate {
	return &gate{
		address: address_,
		port:    port_,
		ses_id:  -1,
		user_id: -1,
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
// Connect - Подключиться к GRPC серверу
//
func (this *gate) Authorize(login, password string) *gates.Result {
	connected, result := this.Status()
	if !connected {
		return result
	}

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	// Нужно ли осуществлять авторизацию
	r, errOpen := this.client.IsAuthRequired(ctx, &grpcgate.Request{})
	if errOpen != nil {
		return this.parseError(errOpen)
	}

	if r.Flag && len(login) > 0 && (consts.AllowEmptyPassword || len(password) > 0) {
		fmt.Printf("- Пытаемся авторизоваться: %s:*****\n", login)

		r, errOpen = this.client.DoOpen(ctx, &grpcgate.Login{Login: login, Password: password, UserId: this.user_id})

		if errOpen != nil {
			this.ses_id = -1
			if consts.ParamDebug {
				utils.Log("[Scd] OPEN", time.Now(), errOpen, true)
			}
			if status.Code(errOpen) == codes.PermissionDenied {
				fmt.Println("- В подключении ", login, ":***** к SCADA серверу отказано: ")
			}
			return this.parseError(errOpen)
		} else {
			sessionJson := make(map[string]int)
			err := json.Unmarshal(r.GetMessage(), &sessionJson)
			if err != nil {
				return gates.NewResult(gates.FailedParseResponse, err, nil)
			}

			this.ses_id = unpackSessionId(int32(sessionJson["obj_id"]))
			fmt.Println("- Индекс сессии сервера (ses_id): ", this.SessionId())
			return gates.NewResult(gates.Authorized, nil, nil)
		}
	}

	if r.Flag && (len(login) == 0 || (!consts.AllowEmptyPassword && len(password) == 0)) {
		return gates.NewResult(gates.FailedAuth, nil, nil)
	}

	return gates.NewResult(gates.Success, nil, nil)
}

func (this *gate) SetUserId(id int32) {
	this.user_id = id
}

//
// Connect - Подключиться к GRPC серверу
//
func (this *gate) Connect() *gates.Result {
	connected, result := this.Status()
	if connected {
		return result
	}

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	// Подключение собственно самого сервера и создание соединения
	var err error
	this.conn, err = grpc.DialContext(ctx,
		consts.ParamGrpcGateHost+":"+strconv.Itoa(consts.ParamGrpcGatePort),
		grpc.WithInsecure(), grpc.WithBlock())

	// grpc.WithTimeout(time.Second*time.Duration(consts.ParamGrpcDialServerTimeOut)

	if err != nil {
		log.Println("- Нет соединения c GRPC сервером: ", err.Error())
		utils.Log("[Scd] GRPC connection", time.Now(), err, true)
		this.conn = nil
		return gates.NewResult(gates.NotConnected, err, nil)
	}

	// Инициализация gRPC драйвера и proto интерфейсов
	this.client = grpcgate.NewGrpcServiceClient(this.conn)
	log.Println("- Контекст получен..")

	// go say("Работает!")

	if consts.ParamDebug {
		utils.Log("[GRPC] OPEN", time.Now(), fmt.Errorf("success"), true)
	}

	fmt.Println("- GRPC шлюз (", this.address, ":", this.port, ") создан..")

	return gates.NewResult(gates.Connected, nil, nil)
}

//
// Формирование obj_id из номера сессии и номера модуля
//
func (this *gate) packObjId(mod_id int32) int32 {
	if this.ses_id < 0 {
		return -1
	}
	return this.ses_id%4<<29 + this.ses_id<<15 + mod_id%4<<13 + mod_id
}

//
// Распаковка номера сессии из obj_id
//
func unpackSessionId(obj_id int32) int32 {
	if obj_id == -1 {
		return -1
	}
	var mask_ses uint32 = 0b00011111111111111000000000000000
	return int32(uint32(obj_id) & mask_ses >> 15)
}

//
// id сессии из obj_id
//
func (this *gate) SessionId() int32 {
	return this.ses_id
}

//
// Вернуть индекс модуля из obj_id
//
func modId(obj_id int32) int32 {
	if obj_id == -1 {
		return -1
	}
	var mask_mod uint32 = 0b00000000000000000001111111111111
	return int32(uint32(obj_id) & mask_mod)
}

//
// Закрыть сессию на сервере
//

func (this *gate) UnAuthorize() *gates.Result {
	fmt.Println("- Деавторизация канала шлюза..")

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	if this.ses_id > 0 {
		_, err := this.client.DoClose(ctx, &grpcgate.Logout{
			ObjId:  this.packObjId(0),
			UserId: this.user_id,
		})

		this.ses_id = -1

		if err != nil {
			return this.parseError(err)
		}

	}

	return gates.NewResult(gates.ConnectionUnathorized, nil, nil)
}

//
// Close - закрыть соединение
//
func (this *gate) Close() error {

	if this.client == nil && this.conn == nil {
		return nil
	}

	fmt.Println("- Остановка и удаление объекта шлюза ..")

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	err := errors.New("success")
	if consts.ParamDebug {
		utils.Log("[GRPC] CLOSE", time.Now(), err, true)
	}

	if this.user_id > 0 {
		gates.New().Del(this.user_id)
	}

	if this.client != nil {
		_, err = this.client.DoClose(ctx, &grpcgate.Logout{
			ObjId: this.packObjId(0),
		})
		this.client = nil
	}

	if this.conn != nil {
		this.conn.Close()
		this.conn = nil
	}

	this.ses_id = -1
	this.user_id = -1

	return err
}

func (this *gate) Status() (bool, *gates.Result) {
	if this.conn != nil && this.conn.GetState() == connectivity.Ready {
		_, result := this.Ping()
		if result.HasError() &&
			(grpc.Code(result.Error()) == codes.NotFound ||
				grpc.Code(result.Error()) == codes.Unavailable) {
			this.ses_id = -1
			fmt.Println("- GRPC шлюз (", this.SessionId(), "|", this.user_id, ")", " не авторизован..")
			return true, gates.NewResult(gates.NotAccess, result.Error(), nil)
		}
		if this.ses_id < 0 {
			fmt.Println("- GRPC шлюз (", this.SessionId(), "|", this.user_id, ")", " подключен, но не авторизован..")
			return true, gates.NewResult(gates.NeedAuhorization, nil, nil)
		}
		fmt.Println("- GRPC шлюз (", this.SessionId(), "|", this.user_id, ")", " подключен и авторизован..")
		return true, gates.NewResult(gates.Authorized, nil, nil)
	}

	return false, gates.NewResult(gates.NotConnected, nil, nil)
}

//
// Ping - проверяет, жив ли сервер
//
func (this *gate) Ping() (bool, *gates.Result) {
	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	r, err := this.client.Ping(ctx, &grpcgate.Request{
		ObjId:  this.packObjId(0),
		UserId: this.user_id,
	})
	if err != nil {
		return false, this.parseError(err)
	}

	return r.Flag, gates.NewResult(gates.Success, nil, nil)
}

//
// DoRequest
//
func (this *gate) DoRequest(command string, message string, mod_id int32) *gates.Result {
	connected, result := this.Status()
	if !connected {
		return result
	}

	if this.ses_id < 0 {
		fmt.Println("- Для получения данных нужна авторизация!")
		return gates.NewResult(gates.NeedAuhorization, nil, nil)
	}

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))
	defer cancel()

	fmt.Println("Obj_id = ", this.packObjId(mod_id))
	r, err := this.client.DoRequest(ctx, &grpcgate.Request{
		ObjId:   this.packObjId(mod_id),
		Command: command,
		Message: []byte(message),
		UserId:  this.user_id,
	})

	if err != nil {
		fmt.Println("- doRequest вернул ошибку: " + err.Error())
		if grpc.Code(err) == codes.Unavailable {
			this.UnAuthorize()
		}
		return this.parseError(err)
	}

	sessionJson := make(map[string]int)
	err = json.Unmarshal(r.GetMessage(), &sessionJson)
	if err == nil {
		if int32(sessionJson["obj_with_mod_id"]) > 0 {
			fmt.Println("- Индекс модуля: ", modId(int32(sessionJson["obj_with_mod_id"])),
				", obj_id с индексом модуля: ", sessionJson["obj_with_mod_id"])
		}
	}

	if len(r.Message) == 0 {
		gates.NewResult(gates.EmptyResponse, errors.New("Ошибка"), nil)
	}

	return gates.NewResult(gates.Success, nil, r.Message)
}

//
// Нужна ли авторизация для шлюза?
//

func (this *gate) AuthRequired() (bool, *gates.Result) {
	connected, result := this.Status()
	if !connected {
		return false, result
	}

	ctx, cancel := context.WithTimeout(context.Background(),
		time.Second*time.Duration(consts.ParamGrpcGateTimeOut))

	defer cancel()

	r, err := this.client.IsAuthRequired(ctx, &grpcgate.Request{})
	if err != nil {
		return false, this.parseError(err)
	}

	if !r.Flag {
		this.ses_id = consts.GuestUserId
	}

	return r.Flag, gates.NewResult(gates.Success, nil, nil)
}

//
// Разбор ошибок. Ошибки сервера приходят в статусе.
//

func (this *gate) parseError(err error) *gates.Result {
	if err == nil {
		return nil
	}

	var code int

	switch grpc.Code(err) {
	case codes.Unavailable:
		code = gates.GrpcUnavalable
	case codes.Internal:
		code = gates.GrpcInternal
	case codes.ResourceExhausted:
		code = gates.GrpcExhausted
	case codes.DeadlineExceeded:
		code = gates.GrpcDeadlineExceed
	case codes.InvalidArgument:
		code = gates.GrpcInvalidArgument
	case codes.NotFound:
		code = gates.GrpcNotFound
	case codes.Unimplemented:
		code = gates.GrpcNotImplemented
	case codes.PermissionDenied:
		code = gates.GrpcPermissionDenied
	case codes.FailedPrecondition:
		code = gates.NeedToSetModule
	case codes.OutOfRange:
		code = gates.ModuleNotLoaded
	default:
		code = gates.GrpcInternal
	}

	return gates.NewResult(code, err, nil)
}
