package modules

import (
	"server/engine/utils"
	"server/engine/wrapper"
)

func (this *Modules) RegisterAction_IndexUserSignIn() *Action {
	return this.newAction(AInfo{
		Mount: "index-user-sign-in",
	}, func(wrap *wrapper.Wrapper) {
		pf_firstname := utils.Trim(wrap.R.FormValue("first_name"))
		pf_password := utils.Trim(wrap.R.FormValue("password"))

		if pf_firstname == "" {
			wrap.MsgError(`Пожалуйста, введите логин`)
			return
		}

		// if !utils.IsValidEmail(pf_email) {
		// 	wrap.MsgError(`Please specify correct user email`)
		// 	return
		// }

		if pf_password == "" && wrap.Config.Modules.Enabled.EmptyPassword == 0 {
			wrap.MsgError(`Пожалуйста, введите пароль`)
			return
		}

		user_id, err := wrap.AuthDirectConnection(pf_firstname, pf_password)
		if err != nil {
			wrap.MsgError("Нет соединения c сервером приложения")
			return
		}

		if wrap.S.GetInt("UserId", 0) > 0 {
			//wrap.MsgError(`Вы уже авторизованы`)
			// TODO: если есть авторизация - обнуляем и делаем редирект на вход
			wrap.S.SetInt("UserId", 0)
			return
		}

		// pass := md5.Sum([]byte(pf_password))

		// var user_id int
		// err := wrap.DB.QueryRow(
		// 	wrap.R.Context(),
		// 	`SELECT
		// 		id
		// 	FROM
		// 		users
		// 	WHERE
		// 		first_name = ? and
		// 		password = ? and
		// 		active = 1
		// 	LIMIT 1;`,
		// 	pf_firstname,
		// 	hex.EncodeToString(pass[:]),
		// ).Scan(
		// 	&user_id,
		// )

		// if err != nil && err != sqlw.ErrNoRows {
		// 	wrap.LogCpError(&err)
		// 	wrap.MsgError(err.Error())
		// 	return
		// }

		// if err == sqlw.ErrNoRows {
		// 	wrap.MsgError(`Некорректны логин или пароль`)
		// 	return
		// }

		// Save to current session
		wrap.S.SetInt("UserId", user_id)

		// Reload current page
		wrap.Write(`window.location.reload(false);`)
	})
}
