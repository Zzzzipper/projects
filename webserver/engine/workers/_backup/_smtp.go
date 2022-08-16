package workers

import (
	"context"
	"fmt"
	"html"
	"io/ioutil"
	"os"
	"strings"
	"time"

	"server/engine/config"
	"server/engine/sqlpool"
	"server/engine/sqlw"
	"server/engine/utils"

	"github.com/vladimirok5959/golang-worker/worker"
)

func SmtpSender(www_dir string, mp *sqlpool.SqlPool) *worker.Worker {
	return worker.New(func(ctx context.Context, w *worker.Worker, o *[]worker.Iface) {
		if www_dir, ok := (*o)[0].(string); ok {
			if mp, ok := (*o)[1].(*sqlpool.SqlPool); ok {
				smtp_loop(ctx, www_dir, mp)
			}
		}
		select {
		case <-ctx.Done():
		case <-time.After(5 * time.Second):
			return
		}
	}, &[]worker.Iface{
		www_dir,
		mp,
	})
}

func smtp_loop(ctx context.Context, www_dir string, mp *sqlpool.SqlPool) {
	dirs, err := ioutil.ReadDir(www_dir)
	if err == nil {
		for _, dir := range dirs {
			select {
			case <-ctx.Done():
				return
			default:
				if mp != nil {
					target_dir := strings.Join([]string{www_dir, dir.Name()}, string(os.PathSeparator))
					if utils.IsDirExists(target_dir) {
						smtp_process(ctx, target_dir, dir.Name(), mp)
					}
				}
			}
		}
	}
}

func smtp_process(ctx context.Context, dir, host string, mp *sqlpool.SqlPool) {
	db := mp.Get(host)
	if db != nil {
		conf := config.ConfigNew()
		if err := conf.ConfigRead(strings.Join([]string{dir, "config", "config.json"}, string(os.PathSeparator))); err == nil {
			if !((*conf).Smtp.Host == "" || (*conf).Smtp.Login == "" && (*conf).Smtp.Password == "") {
				if err := db.Ping(ctx); err == nil {
					smtp_prepare(ctx, db, conf)
				}
			}
		} else {
			fmt.Printf("Smtp error (config): %v\n", err)
		}
	}
}

func smtp_prepare(ctx context.Context, db *sqlw.DB, conf *config.Config) {
	rows, err := db.Query(
		ctx,
		`SELECT
			id,
			email,
			subject,
			message
		FROM
			notify_mail
		WHERE
			status = 2
		ORDER BY
			id ASC
		;`,
	)
	if err == nil {
		defer rows.Close()
		values := make([]string, 4)
		scan := make([]interface{}, len(values))
		for i := range values {
			scan[i] = &values[i]
		}
		for rows.Next() {
			err = rows.Scan(scan...)
			if err == nil {
				if _, err := db.Exec(
					ctx,
					`UPDATE notify_mail SET status = 3 WHERE id = ?;`,
					utils.StrToInt(string(values[0])),
				); err == nil {
					go func(db *sqlw.DB, conf *config.Config, id int, subject, msg string, receivers []string) {
						if err := smtp_send(
							ctx,
							(*conf).Smtp.Host,
							utils.IntToStr((*conf).Smtp.Port),
							(*conf).Smtp.Login,
							(*conf).Smtp.Password,
							subject,
							msg,
							receivers,
						); err == nil {
							if _, err := db.Exec(
								ctx,
								`UPDATE notify_mail SET status = 1 WHERE id = ?;`,
								id,
							); err != nil {
								fmt.Printf("Smtp send error (sql, success): %v\n", err)
							}
						} else {
							if _, err := db.Exec(
								ctx,
								`UPDATE notify_mail SET error = ?, status = 0 WHERE id = ?;`,
								err.Error(),
								id,
							); err != nil {
								fmt.Printf("Smtp send error (sql, error): %v\n", err)
							}
						}
					}(
						db,
						conf,
						utils.StrToInt(string(values[0])),
						html.EscapeString(string(values[2])),
						string(values[3]),
						[]string{html.EscapeString(string(values[1]))},
					)
				} else {
					fmt.Printf("Smtp send error (sql, update): %v\n", err)
				}
			}
		}
	}
}

func smtp_send(ctx context.Context, host, port, user, pass, subject, msg string, receivers []string) error {
	return utils.SMTPSend(host, port, user, pass, subject, msg, receivers)
}
