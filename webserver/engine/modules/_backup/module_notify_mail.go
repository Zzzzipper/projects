package modules

import (
	"html"

	"server/engine/builder"
	"server/engine/consts"
	"server/engine/sqlw"
	"server/engine/utils"
	"server/engine/wrapper"
)

func (this *Modules) RegisterModule_NotifyMail() *Module {
	return this.newModule(MInfo{
		Mount:  "notify-mail",
		Name:   "Почтовый агент",
		Order:  803,
		System: true,
		Icon:   "<i class=\"material-icons notranslate\">alternate_email</i>", //assets.SysSvgIconEmail,
		Sub: &[]MISub{
			{Mount: "default", Name: "Вся почта", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
			{Mount: "success", Name: "Доставленная", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
			{Mount: "in-progress", Name: "В процессе", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
			{Mount: "error", Name: "Ошибка отправки", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
		},
	}, nil, func(wrap *wrapper.Wrapper) (string, string, string) {
		content := ""
		sidebar := ""
		if wrap.CurrSubModule == "" || wrap.CurrSubModule == "default" || wrap.CurrSubModule == "success" || wrap.CurrSubModule == "in-progress" || wrap.CurrSubModule == "error" {
			ModuleName := "All"
			ModulePagination := "/cp/" + wrap.CurrModule + "/"
			ModuleSqlWhere := ""

			if wrap.CurrSubModule == "success" {
				ModuleName = "Success"
				ModulePagination = "/cp/" + wrap.CurrModule + "/success/"
				ModuleSqlWhere = " WHERE notify_mail.status = 1"
			} else if wrap.CurrSubModule == "in-progress" {
				ModuleName = "In progress"
				ModulePagination = "/cp/" + wrap.CurrModule + "/in-progress/"
				ModuleSqlWhere = " WHERE notify_mail.status = 2 OR notify_mail.status = 3"
			} else if wrap.CurrSubModule == "error" {
				ModuleName = "Error"
				ModulePagination = "/cp/" + wrap.CurrModule + "/error/"
				ModuleSqlWhere = " WHERE notify_mail.status = 0"
			}

			content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
				{Name: ModuleName},
			})
			content += builder.DataTable(
				wrap,
				"notify_mail",
				"id",
				"DESC",
				&[]builder.DataTableRow{
					{
						DBField: "id",
					},
					{
						DBField:     "email",
						NameInTable: "Email / Subject",
						CallBack: func(values *[]string) string {
							subject := html.EscapeString((*values)[2])
							if subject != "" {
								subject = `<div><small>` + subject + `</small></div>`
							}
							error_message := html.EscapeString((*values)[5])
							if error_message != "" {
								error_message = `<div><small><b>` + error_message + `</b></small></div>`
							}
							return `<div>` + html.EscapeString((*values)[1]) + `</div>` + subject + error_message
						},
					},
					{
						DBField: "subject",
					},
					{
						DBField: "datetime",
						// DBExp:       "UNIX_TIMESTAMP(`datetime`)",
						DBExp:       "strftime('%s', datetime)",
						NameInTable: "Date / Time",
						Classes:     "d-none d-md-table-cell",
						CallBack: func(values *[]string) string {
							t := int64(utils.StrToInt((*values)[3]))
							return `<div>` + utils.UnixTimestampToFormat(t, "02.01.2006") + `</div>` +
								`<div><small>` + utils.UnixTimestampToFormat(t, "15:04:05") + `</small></div>`
						},
					},
					{
						DBField:     "status",
						NameInTable: "Status",
						Classes:     "d-none d-sm-table-cell",
						CallBack: func(values *[]string) string {
							return builder.CheckBox(utils.StrToInt((*values)[4]))
						},
					},
					{
						DBField: "error",
					},
				},
				nil,
				ModulePagination,
				func() (int, error) {
					var count int
					return count, wrap.DB.QueryRow(
						wrap.R.Context(),
						"SELECT COUNT(*) FROM `notify_mail`"+ModuleSqlWhere+";",
					).Scan(&count)
				},
				func(limit_offset int, pear_page int) (*sqlw.Rows, error) {
					return wrap.DB.Query(
						wrap.R.Context(),
						`SELECT
							notify_mail.id,
							notify_mail.email,
							notify_mail.subject,
							strftime('%s', notify_mail.datetime) as datetime,
							notify_mail.status,
							notify_mail.error
						FROM
							notify_mail
						`+ModuleSqlWhere+`
						ORDER BY
							notify_mail.id DESC
						LIMIT ?, ?;`,
						limit_offset,
						pear_page,
					)
				},
				true,
			)
		}
		return this.getSidebarModules(wrap), content, sidebar
	})
}
