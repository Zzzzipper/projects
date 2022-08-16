package modules

import (
	"encoding/json"
	"fmt"
	"net/http"
	"server/engine/gates"
	"server/engine/wrapper"
	"strconv"

	"github.com/golang/gddo/httputil/header"
)

// Блокировщик слишком частых запросов авторизации (например, doubleClick)
var authLocker = make(chan bool, 1)

type Credential struct {
	Login    string `json:login`
	Password string `json:password`
}

func lockAuth() {
	authLocker <- true
}

func unlockAuth() {
	<-authLocker
}

func isLockAuth() bool {
	return len(authLocker) > 0
}

func (this *Modules) RegisterData_Auth() *Data {
	return this.newData(DInfo{
		Mount: "login",
	},
		func(wrap *wrapper.Wrapper) {

			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("- Адрес клиента: ", wrap.R.Header["Origin"])
				wrap.W.Header().Add("Access-Control-Allow-Origin", wrap.R.Header["Origin"][0])
			}
			wrap.W.Header().Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			wrap.W.Header().Add("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
			wrap.W.Header().Add("Access-Control-Allow-Credentials", "true")

			if wrap.R.Header.Get("Content-Type") != "" {
				value, _ := header.ParseValueAndParams(wrap.R.Header, "Content-Type")
				if value != "application/json" {
					msg := "Content-Type header is not application/json"
					wrap.W.Write([]byte("{\"error\": \"" + msg + ", " + strconv.Itoa(http.StatusUnsupportedMediaType) + "\"}"))
					return
				}
			}

			wrap.R.Body = http.MaxBytesReader(wrap.W, wrap.R.Body, 1048576)

			// Проверка, выполняется ли предыдущий запроса авторизации
			if isLockAuth() {
				wrap.W.Write([]byte(gates.NewResult(gates.PrevAuthNotCompleted, nil, nil).Json()))
				return
			}

			lockAuth()
			defer unlockAuth()

			dec := json.NewDecoder(wrap.R.Body)
			dec.DisallowUnknownFields()

			var p Credential
			err := dec.Decode(&p)

			if err != nil {
				wrap.W.Write([]byte(gates.NewResult(gates.HTTPPostError, nil, nil).Message()))
				return
			}

			// Индекс текущего пользователя  - сохраним
			user_id := wrap.S.GetInt("UserId", -1)

			// Проверка канала
			gate := gates.New().Get(user_id)
			if gate != nil {
				status, result := gate.(gates.IGate).AuthRequired()
				if !status {
					if result.HasError() {
						fmt.Println("- ", result.Message())
						wrap.W.Write([]byte(result.Json()))
					} else {
						wrap.W.Write([]byte(gates.NewResult(gates.NotNeedAuthorization, nil, nil).Json()))
						return
					}
				} else {
					if gate.(gates.IGate).SessionId() > 0 {
						wrap.W.Write([]byte(gates.NewResult(gates.NeedCloseSession, nil, nil).Json()))
						return
					} else {
						result := gate.(gates.IGate).Authorize(p.Login, p.Password)
						wrap.W.Write([]byte(result.Json()))
					}
				}
			}
		})
}
