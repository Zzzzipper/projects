package modules

import (
	"context"
	"crypto/md5"
	"encoding/hex"

	"server/engine/config"
	"server/engine/utils"
	"server/engine/wrapper"
)

func (this *Modules) RegisterAction_UsersModify() *Action {
	return this.newAction(AInfo{
		Mount:     "users-modify",
		WantAdmin: true,
	}, func(wrap *wrapper.Wrapper) {
		pf_id := utils.Trim(wrap.R.FormValue("id"))
		pf_first_name := utils.Trim(wrap.R.FormValue("first_name"))
		pf_last_name := utils.Trim(wrap.R.FormValue("last_name"))
		pf_email := utils.Trim(wrap.R.FormValue("email"))
		pf_password := utils.Trim(wrap.R.FormValue("password"))
		pf_admin := utils.Trim(wrap.R.FormValue("admin"))
		pf_active := utils.Trim(wrap.R.FormValue("active"))
		pf_address := utils.Trim(wrap.R.FormValue("address"))
		pf_port := utils.Trim(wrap.R.FormValue("port"))

		if pf_admin == "" {
			pf_admin = "0"
		}

		if pf_active == "" {
			pf_active = "0"
		}

		if !utils.IsNumeric(pf_id) {
			wrap.MsgError(`Inner system error`)
			return
		}

		if pf_email == "" {
			wrap.MsgError(`Пожалуйста, введите email поль`)
			return
		}

		if !utils.IsValidEmail(pf_email) {
			wrap.MsgError(`Пожалуйста, введите корректный email пользователя`)
			return
		}

		// First user always super admin
		// Rewrite active and admin status
		if pf_id == "1" {
			pf_admin = "1"
			pf_active = "1"
		}

		if pf_id == "0" {
			// Add new user
			if pf_password == "" && config.ConfigNew().Modules.Enabled.EmptyPassword != 1 {
				wrap.MsgError(`Пожалуйста, задайте пароль пользователя`)
				return
			}

			pf_signature := utils.Encrypt([]byte(pf_password), nil)
			pass := md5.Sum([]byte(pf_password))

			var lastID int64 = 0
			if err := wrap.DB.Transaction(wrap.R.Context(), func(ctx context.Context, tx *wrapper.Tx) error {
				res, err := tx.Exec(
					ctx,
					`INSERT INTO users (
						first_name,
						last_name,
						email,
						password,
						admin,
						active,
						address,
						port,
						signature
					) VALUES (
						?,
						?,
						?,
						?,
						?,
						?,
						?,
						?,
						?
					)
					;`,
					pf_first_name,
					pf_last_name,
					pf_email,
					hex.EncodeToString(pass[:]),
					pf_admin,
					utils.StrToInt(pf_active),
					pf_address,
					utils.StrToInt(pf_port),
					pf_signature,
				)

				if err != nil {
					return err
				}
				// Get inserted post id
				lastID, err = res.LastInsertId()
				if err != nil {
					return err
				}
				return nil
			}); err != nil {
				wrap.MsgError(err.Error())
				return
			}
			wrap.ResetCacheBlocks()
			wrap.Write(`window.location='/cp/users/modify/` + utils.Int64ToStr(lastID) + `/';`)
		} else {
			// Update user
			if pf_password == "" {
				if err := wrap.DB.Transaction(wrap.R.Context(), func(ctx context.Context, tx *wrapper.Tx) error {
					_, err := tx.Exec(
						ctx,
						`UPDATE users SET
							first_name = ?,
							last_name = ?,
							email = ?,
							admin = ?,
							active = ?,
							address = ?,
							port = ?
						WHERE
							id = ?
						;`,
						pf_first_name,
						pf_last_name,
						pf_email,
						pf_admin,
						utils.StrToInt(pf_active),
						pf_address,
						utils.StrToInt(pf_port),
						utils.StrToInt(pf_id),
					)
					if err != nil {
						return err
					}
					return nil
				}); err != nil {
					wrap.MsgError(err.Error())
					return
				}
			} else {

				pf_signature := utils.Encrypt([]byte(pf_password), nil)
				pass := md5.Sum([]byte(pf_password))

				if err := wrap.DB.Transaction(wrap.R.Context(), func(ctx context.Context, tx *wrapper.Tx) error {
					_, err := tx.Exec(
						ctx,
						`UPDATE users SET
							first_name = ?,
							last_name = ?,
							email = ?,
							password = ?,
							address = ?,
							port = ?,
							signature = ?
						WHERE
							id = ?
						;`,
						pf_first_name,
						pf_last_name,
						pf_email,
						hex.EncodeToString(pass[:]),
						pf_address,
						utils.StrToInt(pf_port),
						pf_signature,
						utils.StrToInt(pf_id),
					)
					if err != nil {
						return err
					}
					return nil
				}); err != nil {
					wrap.MsgError(err.Error())
					return
				}
			}
			wrap.ResetCacheBlocks()
			wrap.Write(`window.location='/cp/users/modify/` + pf_id + `/';`)
		}
	})
}
