package migrate

import (
	"context"
	"fmt"

	"server/engine/sqlw"
	"server/engine/utils"
)

func Exec(ctx context.Context, db *sqlw.DB, version int, host string) error {
	var last string
	for i, fn := range Migrations {
		if utils.StrToInt(i) > 1 {
			if version < utils.StrToInt(i) {
				last = i
				if fn != nil {
					err := fn(ctx, db, host)
					fmt.Printf("Migrated %s: %s, error: %s\n", host, i, err.Error())
				}
			}
		}
	}

	if last != "" {
		if _, err := db.Exec(ctx, `UPDATE settings SET value = ? WHERE name = 'database_version';`, last); err != nil {
			return err
		}
	}

	return nil
}
