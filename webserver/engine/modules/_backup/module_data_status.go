package modules

import (
	"fmt"
	"server/engine/wrapper"
)

func (this *Modules) RegisterData_Status() *Data {
	return this.newData(DInfo{
		Mount: "status",
	},
		func(wrap *wrapper.Wrapper) {
			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("Origin for status: ", wrap.R.Header["Origin"])
				wrap.W.Header().Add("Access-Control-Allow-Origin", wrap.R.Header["Origin"][0])
			}
			wrap.W.Header().Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			wrap.W.Header().Add("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
			wrap.W.Header().Add("Access-Control-Allow-Credentials", "true")

			// var first_name string
			// err := wrap.DB.QueryRow(
			// 	wrap.R.Context(),
			// 	`SELECT
			// 	first_name
			// FROM
			// 	users
			// WHERE
			// 	id = ?
			// LIMIT 1;`,
			// 	wrap.S.GetInt("UserId", 0),
			// ).Scan(
			// 	&first_name,
			// )

			// if err != nil && err != sqlw.ErrNoRows {
			// 	wrap.W.Write([]byte("{\"error\": \"" + err.Error() + "\"}"))
			// 	return
			// }

			// if err == sqlw.ErrNoRows {
			// 	wrap.W.Write([]byte("{\"loggedin\":\"nothing\"}"))
			// 	return
			// }

			// wrap.W.Write([]byte("{\"loggedin\":\"" + first_name + "\"}"))
			// return
			if wrap.User != nil {
				wrap.W.Write([]byte("{\"loggedin\":\"" + wrap.User.A_first_name + "\"}"))
			} else {
				wrap.W.Write([]byte("{\"loggedin\":\"nothing\"}"))
			}
		})
}
