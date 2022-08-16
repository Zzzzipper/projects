package wrapper

import (
	"bytes"
	"fmt"
	"html/template"
	"net/http"
	"server/engine/consts"
	"server/engine/gates"
	"server/engine/gates/grpc"
	"strconv"

	"server/engine/cblocks"
	"server/engine/logger"
	"server/engine/utils"

	"server/engine/session"
)

type Wrapper struct {
	l                 *logger.Logger
	W                 http.ResponseWriter
	R                 *http.Request
	S                 *session.Session
	c                 *cblocks.CacheBlocks
	Host              string
	Port              string
	CurrHost          string
	DHtdocs           string
	DLogs             string
	DTmp              string
	IsBackend         bool
	ConfMysqlExists   bool
	UrlArgs           []string
	CurrModule        string
	CurrSubModule     string
	CurrDashboard     string
	GatePool          *gates.Pool
	ShopAllCurrencies *map[int]utils.Sql_shop_currency
}

func New(l *logger.Logger, w http.ResponseWriter, r *http.Request, s *session.Session, c *cblocks.CacheBlocks, host, port, chost, dirHtdocs, dirLogs, dirTmp string, gates *gates.Pool) *Wrapper {
	return &Wrapper{
		l:             l,
		W:             w,
		R:             r,
		S:             s,
		c:             c,
		Host:          host,
		Port:          port,
		CurrHost:      chost,
		DHtdocs:       dirHtdocs,
		DLogs:         dirLogs,
		DTmp:          dirTmp,
		UrlArgs:       []string{},
		CurrModule:    "",
		CurrSubModule: "",
		GatePool:      gates,
	}
}

func (this *Wrapper) LogAccess(msg string, vars ...interface{}) {
	this.l.Log(msg, this.R, false, vars...)
}

func (this *Wrapper) LogError(msg string, vars ...interface{}) {
	this.l.Log(msg, this.R, true, vars...)
}

func (this *Wrapper) LogCpError(err *error) *error {
	if *err != nil {
		this.LogError("%s", *err)
		fmt.Println("LogCpError: ", *err)
	}
	return err
}

func (this *Wrapper) grpcConnect(login string, password string) (int32, error) {
	var gate gates.IGate = grpc.NewGate(
		consts.ParamGrpcGateHost,
		uint(consts.ParamGrpcGatePort))

	// err := gate.Connect()
	// if err != nil {
	// 	return -1, err
	// }

	// this.GatePool.Set(gate.SessionId(), gate)

	// fmt.Printf("# GRPC gate creation: %v\n", gate)

	return gate.SessionId(), nil
}

// Закрытие пользовательского соединения с сервером,
// если это соединение ранее было создано и у него нет статуса готовности.
//
// Такая ситуация возникает, например, если в процессе работы по каким-то причинам
// сервер отключился.
func (wrap *Wrapper) closeConnectionIfUnready() {
	user_id := wrap.S.GetInt("UserId", -1)
	if gate := wrap.GatePool.Get(user_id); gate != nil {
		if isConnect, _ := gate.(gates.IGate).Status(); !isConnect {
			gate.(gates.IGate).Close()
		}
	}
}

func (this *Wrapper) AuthDirectConnection(login string, password string) (int32, error) {
	// Проверяем индекс пользователя сессии и актуальность соединения
	var user_id int32 = this.S.GetInt("UserId", -1)
	// var err error
	if user_id <= 0 {
		fmt.Println("- UserId не инициализирован")
	} else {
		fmt.Println("- UserId есть: ", user_id)
	}

	gate := gates.New().Get(user_id)
	if gate == nil {
		fmt.Printf("- Шлюз для пользователя id:%d не создан\n", user_id)
	} else {
		fmt.Printf("- Шлюз для пользователя id:%d создан\n", user_id)
	}

	// if user_id <= 0 || gates.New().Get(user_id) == nil {
	// 	if user_id, err = this.grpcReconnect(login, password); err != nil {
	// 		return -1, err
	// 	}
	// }
	return user_id, nil

}

func (this *Wrapper) UseGatesConnections() *gates.Result {
	if !consts.HasGrpcGate {
		return gates.NewResult(gates.GrpcNotDefined, nil, nil)
	}

	this.closeConnectionIfUnready()

	user_id := this.S.GetInt("UserId", -1)
	// Создаем соединение для пользователя, если его нет
	if this.GatePool.Get(user_id) == nil {
		var gate gates.IGate = grpc.NewGate(
			consts.ParamGrpcGateHost,
			uint(consts.ParamGrpcGatePort))
		result := gate.Connect()
		if result.HasError() {
			return result
		}
		required, result := gate.AuthRequired()
		this.GatePool.Set(user_id, gate)
		gate.SetUserId(user_id)
		if !required {
			if result.HasError() {
				return result
			}
		} else {
			return gates.NewResult(gates.NeedAuhorization, nil, nil)
		}
	}

	return gates.NewResult(gates.Connected, nil, nil)
}

func (this *Wrapper) LoadSessionUser() bool {
	session_id := this.S.GetInt("UserId", -1)
	if session_id <= 0 {
		return false
	}

	// gate := this.GatePool.Get(session_id)
	// if gate == nil {
	// 	this.S.SetInt("UserId", -1)
	// 	return false
	// } else {
	// 	this.User = &utils.Sql_user{
	// 		A_first_name: gate.(gates.IGate).Login(),
	// 		A_password:   gate.(gates.IGate).Password(),
	// 	}
	// }
	return true
}

func (this *Wrapper) Write(data string) {
	// TODO: select context and don't write
	this.W.Write([]byte(data))
}

func (this *Wrapper) MsgSuccess(msg string) {
	// TODO: select context and don't write
	this.Write(fmt.Sprintf(
		`fave.ShowMsgSuccess('Success!', '%s', false);`,
		utils.JavaScriptVarValue(msg)))
}

func (this *Wrapper) MsgError(msg string) {
	// TODO: select context and don't write
	this.Write(fmt.Sprintf(
		`fave.ShowMsgError('Error!', '%s', true);`,
		utils.JavaScriptVarValue(msg)))
}

func (this *Wrapper) RenderToString(tcont []byte, data interface{}) string {
	tmpl, err := template.New("template").Parse(string(tcont))
	if err != nil {
		return err.Error()
	}
	var tpl bytes.Buffer
	if err := tmpl.Execute(&tpl, data); err != nil {
		return err.Error()
	}
	return tpl.String()
}

func (this *Wrapper) GetCurrentPage(max int) int {
	curr := 1
	page := this.R.URL.Query().Get("p")
	if page != "" {
		if i, err := strconv.Atoi(page); err == nil {
			if i < 1 {
				curr = 1
			} else if i > max {
				curr = max
			} else {
				curr = i
			}
		}
	}
	return curr
}

func (this *Wrapper) GetSessionId() string {
	cookie, err := this.R.Cookie("session")
	if err == nil && len(cookie.Value) == 40 {
		return cookie.Value
	}
	return ""
}
