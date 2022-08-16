package modules

import (
	"html"
	"strconv"

	"server/engine/assets"
	"server/engine/builder"
	"server/engine/consts"
	"server/engine/utils"
	"server/engine/wrapper"
)

func (this *Modules) RegisterModule_Users() *Module {
	return this.newModule(MInfo{
		Mount:  "users",
		Name:   "Пользователи",
		Order:  800,
		System: true,
		Icon:   "<i class=\"material-icons notranslate\">people</i>", //assets.SysSvgIconUser,
		Sub: &[]MISub{
			{Mount: "default", Name: "Список пользователей", Show: true, Icon: "<i class=\"material-icons notranslate\">list</i>" /*assets.SysSvgIconList*/},
			{Mount: "add", Name: "Добавить", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
			{Mount: "modify", Name: "Редактировать", Show: false},
		},
	}, nil, func(wrap *wrapper.Wrapper) (string, string, string) {
		content := ""
		sidebar := ""
		if wrap.CurrSubModule == "" || wrap.CurrSubModule == "default" {
			content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
				{Name: "Список пользователей"},
			})
			content += builder.DataTable(
				wrap,
				"users",
				"id",
				"DESC",
				&[]builder.DataTableRow{
					{
						DBField: "id",
					},
					{
						DBField:     "email",
						NameInTable: "Email / Name",
						CallBack: func(values *[]string) string {
							email := `<a href="/cp/` + wrap.CurrModule + `/modify/` + (*values)[0] + `/">` + html.EscapeString((*values)[1]) + `</a>`
							name := html.EscapeString((*values)[2])
							if name != "" && (*values)[3] != "" {
								name += ` ` + (*values)[3]
							}
							if name != "" {
								name = `<div><small>` + name + `</small></div>`
							}
							return `<div>` + email + `</div>` + name
						},
					},
					{
						DBField: "first_name",
					},
					{
						DBField: "last_name",
					},
					{
						DBField:     "active",
						NameInTable: "Активирован",
						Classes:     "d-none d-sm-table-cell",
						CallBack: func(values *[]string) string {
							return builder.CheckBox(utils.StrToInt((*values)[4]))
						},
					},
					{
						DBField:     "admin",
						NameInTable: "Администратор",
						Classes:     "d-none d-md-table-cell",
						CallBack: func(values *[]string) string {
							return builder.CheckBox(utils.StrToInt((*values)[5]))
						},
					},
				},
				func(values *[]string) string {
					return builder.DataTableAction(&[]builder.DataTableActionRow{
						{
							Icon: assets.SysSvgIconEdit,
							Href: "/cp/" + wrap.CurrModule + "/modify/" + (*values)[0] + "/",
							Hint: "Редактировать",
						},
						{
							Icon: assets.SysSvgIconRemove,
							Href: "javascript:fave.ActionDataTableDelete(this,'users-delete','" +
								(*values)[0] + "','Are you sure want to delete user?');",
							Hint:    "Удалить",
							Classes: "delete",
						},
					})
				},
				"/cp/"+wrap.CurrModule+"/",
				nil,
				nil,
				true,
			)
		} else if wrap.CurrSubModule == "add" || wrap.CurrSubModule == "modify" {
			if wrap.CurrSubModule == "add" {
				content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
					{Name: "Добавить пользователя"},
				})
			} else {
				content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
					{Name: "Редактировать профиль"},
				})
			}

			data := utils.Sql_user{
				A_id:         0,
				A_first_name: "",
				A_last_name:  "",
				A_email:      "",
				A_address:    "127.0.0.1",
				A_port:       1312,
				A_admin:      0,
				A_active:     0,
			}

			if wrap.CurrSubModule == "modify" {
				if len(wrap.UrlArgs) != 3 {
					return "", "", ""
				}
				if !utils.IsNumeric(wrap.UrlArgs[2]) {
					return "", "", ""
				}
				err := wrap.DB.QueryRow(
					wrap.R.Context(),
					`SELECT
						id,
						first_name,
						last_name,
						email,
						admin,
						active,
						address,
						port
					FROM
						users
					WHERE
						id = ?
					LIMIT 1;`,
					utils.StrToInt(wrap.UrlArgs[2]),
				).Scan(
					&data.A_id,
					&data.A_first_name,
					&data.A_last_name,
					&data.A_email,
					&data.A_admin,
					&data.A_active,
					&data.A_address,
					&data.A_port,
				)
				if *wrap.LogCpError(&err) != nil {
					return "", "", ""
				}
			}

			pass_req := true
			pass_hint := ""
			if wrap.CurrSubModule == "modify" {
				pass_req = false
				pass_hint = "Оставьте это поле пустым, если не хотите изменить пароль"
			}

			btn_caption := "Добавить"
			if wrap.CurrSubModule == "modify" {
				btn_caption = "Сохранить"
			}

			content += builder.DataForm(wrap, []builder.DataFormField{
				{
					Kind:  builder.DFKHidden,
					Name:  "action",
					Value: "users-modify",
				},
				{
					Kind:  builder.DFKHidden,
					Name:  "id",
					Value: utils.IntToStr(data.A_id),
				},
				{
					Kind:    builder.DFKText,
					Caption: "Логин",
					Name:    "first_name",
					Value:   data.A_first_name,
				},
				{
					Kind:    builder.DFKText,
					Caption: "Алиас",
					Name:    "last_name",
					Value:   data.A_last_name,
				},
				{
					Kind:     builder.DFKEmail,
					Caption:  "Email",
					Name:     "email",
					Value:    data.A_email,
					Required: true,
				},
				{
					Kind:     builder.DFKPassword,
					Caption:  "Пароль",
					Name:     "password",
					Required: pass_req,
					Hint:     pass_hint,
				},
				{
					Kind:    builder.DFKIpAddr,
					Caption: "IP Адрес сервера приложений",
					Name:    "address",
					Value:   data.A_address,
				},
				{
					Kind:    builder.DFKIpPort,
					Caption: "Порт сервера",
					Name:    "port",
					Value:   strconv.Itoa(int(data.A_port)),
				},
				{
					Kind:    builder.DFKCheckBox,
					Caption: "Активирован",
					Name:    "active",
					Value:   utils.IntToStr(data.A_active),
				},
				{
					Kind:    builder.DFKCheckBox,
					Caption: "Администратор",
					Name:    "admin",
					Value:   utils.IntToStr(data.A_admin),
				},
				{
					Kind:   builder.DFKSubmit,
					Value:  btn_caption,
					Target: "add-edit-button",
				},
			})

			if wrap.CurrSubModule == "add" {
				sidebar += `<button class="btn btn-primary btn-sidebar" id="add-edit-button">Add</button>`
			} else {
				sidebar += `<button class="btn btn-primary btn-sidebar" id="add-edit-button">Save</button>`
			}
		}
		return this.getSidebarModules(wrap), content, sidebar
	})
}
