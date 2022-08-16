package support

import (
	"context"
	"io/ioutil"
	"os"
	"regexp"
	"strings"

	"server/engine/config"
	"server/engine/sqlw"
	"server/engine/utils"
)

type Support struct {
}

func New() *Support {
	sup := Support{}
	return &sup
}

func (this *Support) isSettingsTableDoesntExist(err error) bool {
	error_msg := strings.ToLower(err.Error())
	if match, _ := regexp.MatchString(`^error 1146`, error_msg); match {
		if match, _ := regexp.MatchString(`'[^\.]+\.settings'`, error_msg); match {
			if match, _ := regexp.MatchString(`doesn't exist$`, error_msg); match {
				return true
			}
		}
	}
	if err.Error() == "no such table: settings" {
		return true
	}
	return false
}

func (this *Support) Migration(ctx context.Context, dir string) error {
	// fmt.Printf("[MIGRATION] PLEASE WAIT UNTIL THIS WILL BE DONE!\n")
	// fmt.Println("Dir: ", dir)
	files, err := ioutil.ReadDir(dir)
	if err != nil {
		return err
	}
	for _, file := range files {
		if utils.IsDir(dir + string(os.PathSeparator) + file.Name()) {
			if err := this.Migrate(ctx, dir+string(os.PathSeparator)+file.Name()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (this *Support) Migrate(ctx context.Context, host string) error {

	config := config.ConfigNew()
	config.ConfigRead(host + string(os.PathSeparator) + "config" + string(os.PathSeparator) + "config.json")

	sql_config_file := host + string(os.PathSeparator) + "config" + string(os.PathSeparator) + config.DbConection.CurrDriver + ".json"
	if utils.IsConfigExists(sql_config_file) {
		mc, err := utils.SqlConfigRead(sql_config_file)
		if err != nil {
			return err
		}
		db, err := sqlw.Open(config.DbConection.CurrDriver, utils.MakeConnectionString(config.DbConection.CurrDriver, mc))
		if err != nil {
			return err
		}
		if err := db.Ping(ctx); err != nil {
			return err
		}
		defer db.Close()

		var table string
		if err := db.QueryRow(ctx, `SHOW TABLES LIKE 'settings';`).Scan(&table); err == nil {
			if table == "settings" && config.DbConection.CurrDriver != "sqlite3" {
				if _, err := db.Exec(ctx, `RENAME TABLE settings TO settings.old;`); err != nil {
					return err
				}
			}
		}

		var version string
		if err := db.QueryRow(ctx, `SELECT value FROM settings WHERE name = 'database_version' LIMIT 1;`).Scan(&version); err != nil {
			if this.isSettingsTableDoesntExist(err) {
				switch config.DbConection.CurrDriver {
				case "mysql":
					{
						if _, err := db.Exec(
							ctx,
							`CREATE TABLE settings (
								name varchar(255) NOT NULL COMMENT 'Setting name',
								value text NOT NULL COMMENT 'Setting value'
							) ENGINE=InnoDB DEFAULT CHARSET=utf8;`,
						); err != nil {
							return err
						}
					}
					break
				case "sqlite3":
					{
						if _, err := db.Exec(
							ctx,
							`CREATE TABLE settings (
								name varchar(255) NOT NULL, -- Setting name
								value text NOT NULL -- Setting value
							);`,
						); err != nil {
							return err
						}
						// Now initialize database
						err = InitDataBase(ctx, db)
						if err != nil {
							return err
						}

					}
					break
				}
				if _, err := db.Exec(
					ctx,
					`INSERT INTO settings (name, value) VALUES ('database_version', '000000002');`,
				); err != nil {
					return err
				}
				version = "000000002"
				err = nil
			}
			return err
		}
		// return this.Process(ctx, db, version, host)
	}
	return nil
}

// func (this *Support) Process(ctx context.Context, db *sqlw.DB, version string, host string) error {
// 	return migrate.Run(ctx, db, utils.StrToInt(version), host)
// }
