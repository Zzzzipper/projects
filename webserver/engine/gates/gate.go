package gates

import "fmt"

const (
	PrevAuthNotCompleted  = -27 // Предыдущий запрос авторизации еще не завершился
	ModuleNotLoaded       = -26 // Модуль не загружен
	EmptyResponse         = -25 // Пустой ответ от GRPC сервера
	GrpcPermissionDenied  = -23 // Доступ запрещен
	GrpcUnknown           = -22 // Неизвестная ошибка от GRPC шлюза
	GrpcNotImplemented    = -21 // Функционал не реализован
	GrpcNotFound          = -20 // Сервис не найден
	GrpcInvalidArgument   = -19 // Неправильный формат аргументов
	GrpcInternal          = -18 // Внутренняя ошибка слоя приложения
	GrpcDeadlineExceed    = -17 // Превышено время выполнения операции
	GrpcExhausted         = -16 // Не достаточно ресурсов для создания объекта
	NeedToSetModule       = -14 // Прежде чем запрашивать данные, необходимо загрузить модуль
	ScadaIsClosed         = -13 // Проверьте доступность сервера приложения
	NeedCloseSession      = -12 // Сначала необходимо закрыть сессию
	HTTPPostError         = -10 // Ошибка в параметрах POST запроса
	GrpcNotDefined        = -8  // Параметры GRPС шлюза не указаны в параметрах запуска
	FailedParseResponse   = -7  // Ошибка разбора ответа от GRPC сервера
	FailedAuth            = -6  // Ошибка авторизации
	GrpcUnavalable        = -4  // Сервис не доступен
	NotAccess             = -3  // Нет доступа
	NeedAuhorization      = -2  // Для получения данных нужна авторизация
	NotConnected          = -1  // Нет соединения с GRPC сервером
	Success               = 0   // OK
	Connected             = 5   // Подключение создано
	Authorized            = 9   // Сессия авторизована
	NotNeedAuthorization  = 11  // Авторизация не нужна
	ModuleIsSet           = 15  // Модуль загружен
	ConnectionUnathorized = 24  // Сессия закрыта
)

var message = map[int]string{
	-27: "Предыдущий запрос авторизации еще не завершился",
	-26: "Модуль не загружен",
	-25: "Пустой ответ от GRPC сервера",
	-23: "Доступ запрещен",
	-22: "Неизвестная ошибка от GRPC шлюза",
	-21: "Функционал не реализован",
	-20: "Сервис не найден",
	-19: "Неправильный формат аргументов",
	-18: "Внутренняя ошибка слоя приложения",
	-17: "Превышено время выполнения операции",
	-16: "Не достаточно ресурсов для создания объекта",
	-14: "Прежде чем запрашивать данные, необходимо загрузить модуль",
	-13: "Проверьте доступность сервера приложения",
	-12: "Сначала необходимо закрыть сессию",
	-10: "Ошибка в параметрах POST запроса",
	-8:  "Параметры GRPС шлюза не указаны в параметрах запуска",
	-7:  "Ошибка разбора ответа от GRPC сервера",
	-6:  "Ошибка авторизации",
	-4:  "Сервис не доступен",
	-3:  "Нет доступа",
	-2:  "Для получения данных нужна авторизация",
	-1:  "Нет соединения с GRPC сервером",
	0:   "OK",
	5:   "Подключение создано",
	9:   "Сессия авторизована",
	11:  "Авторизация не нужна",
	15:  "Модуль загружен",
	24:  "Сессия закрыта",
}

type Result struct {
	err  error
	id   int
	body []byte
}

type IGate interface {
	Connect() *Result
	Authorize(login, password string) *Result
	UnAuthorize() *Result
	Ping() (bool, *Result)
	DoRequest(command string, message string, mod_id int32) *Result
	Close() error
	SessionId() int32
	AuthRequired() (bool, *Result)
	Status() (bool, *Result)
	SetUserId(id int32)
}

func NewResult(id_ int, err_ error, body_ []byte) *Result {
	return &Result{
		err:  err_,
		id:   id_,
		body: body_,
	}
}

func (this *Result) Id() int {
	return this.id
}

func (this *Result) Error() error {
	return this.err
}

func (this *Result) HasError() bool {
	if this.err != nil {
		return true
	}
	return false
}

func (this *Result) Message() string {
	mes := message[this.id]
	if this.err != nil {
		mes += ", error: " + string(this.err.Error())
	}
	return mes
}

//
// Упаковываетв JSON
//

func (this *Result) Json() []byte {
	if this.body != nil {
		return []byte(fmt.Sprintf("{\"code\":%d, \"message\":\"%s\",\"body\":%s}",
			this.id, this.Message(), string(this.body)))
	}
	return []byte(fmt.Sprintf("{\"code\":%d, \"message\":\"%s\"}",
		this.id, this.Message()))
}
