package utils

import (
	"fmt"
	"os"
	"regexp"
	"server/engine/consts"
	"strings"
	"time"
)

func Log(query string, s time.Time, e error, transaction bool) {
	if !consts.ParamDebug {
		return
	}
	msg := query
	if reg, err := regexp.Compile("[\\s\\t]+"); err == nil {
		msg = strings.Trim(reg.ReplaceAllString(msg, " "), " ")
	}
	if reg, err := regexp.Compile("[\\s\\t]+;$"); err == nil {
		msg = reg.ReplaceAllString(msg, ";")
	}
	eStr := " (nil)"
	if e != nil {
		eStr = " \033[0m\033[0;31m(" + e.Error() + ")"
	}
	if consts.IS_WIN {
		fmt.Fprintln(os.Stdout, "[GATES] "+msg+eStr+fmt.Sprintf(" %.3f ms", time.Now().Sub(s).Seconds()))
	} else {
		color := "0;33"
		if transaction {
			color = "1;33"
		}
		fmt.Fprintln(os.Stdout, "\033["+color+"m[GATES] "+msg+eStr+fmt.Sprintf(" %.3f ms", time.Now().Sub(s).Seconds())+"\033[0m")
	}
}
