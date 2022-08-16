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

func (this *Modules) RegisterData_Lockids() *Data {
	return this.newData(DInfo{
		Mount: "lockids",
	},
		func(wrap *wrapper.Wrapper) {

			if wrap.R.Header.Get("Content-Type") != "" {
				value, _ := header.ParseValueAndParams(wrap.R.Header, "Content-Type")
				fmt.Println("RegisterData_Auth Content-type:", value)
				if value != "application/json" {
					msg := "Content-Type header is not application/json"
					wrap.W.Write([]byte("{\"error\": \"" + msg + ", " + strconv.Itoa(http.StatusUnsupportedMediaType) + "\"}"))
					return
				}
			}

			wrap.R.Body = http.MaxBytesReader(wrap.W, wrap.R.Body, 1048576)

			dec := json.NewDecoder(wrap.R.Body)
			dec.DisallowUnknownFields()

			var m Measuring
			err := dec.Decode(&m)

			if err != nil && wrap.R.Method != "OPTIONS" {
				wrap.W.Write([]byte("{\"error\": " + err.Error() + "}"))
				return
			}

			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("Origin for lockids: ", wrap.R.Header["Origin"])
				wrap.W.Header().Add("Access-Control-Allow-Origin", wrap.R.Header["Origin"][0])
			}

			wrap.W.Header().Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			wrap.W.Header().Add("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
			wrap.W.Header().Add("Access-Control-Allow-Credentials", "true")
			wrap.W.Header().Add("Content-Type", "application/json; charset=utf-8")

			if wrap.R.Method == "OPTIONS" {
				this.XXXActionHeaders(wrap, http.StatusOK)
				return
			}

			//
			// TODO: исправить поиск шлюза в зависимости о запроса
			//
			conn := wrap.GatePool.Get("scdgate", wrap.S.GetInt("UserId", -1))
			if conn != nil {
				lockidsData, err := conn.(gates.IGate).LockIds(m.Ids)
				if err != nil {
					lockidsData = "{\"error\":\"" + err.Error() + "\"}"
				}
				this.XXXActionHeaders(wrap, http.StatusOK)
				wrap.W.Write([]byte(lockidsData))

			} else {
				wrap.W.Write([]byte("{\"error\":\"not connected\"}"))
			}

		})
}
