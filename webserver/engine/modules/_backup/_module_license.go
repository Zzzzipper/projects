package modules

import (
	"html"
	"io/ioutil"
	"net/http"
	"server/engine/assets"
	"server/engine/builder"
	"server/engine/consts"
	"server/engine/fetdata"
	"server/engine/utils"
	"server/engine/wrapper"
	"strings"
)

func (this *Modules) dashboard_TemplateNameToValue(filename string) string {
	if i := strings.LastIndex(filename, "."); i > -1 {
		return filename[:i]
	}
	return filename
}

func (this *Modules) dashboard_GetTemplateSelectOptions(wrap *wrapper.Wrapper, template string) string {
	result := ``

	//dashboard.html
	result += `<option title="dashboard.html" value="dashboard"`
	if template == "dashboard" {
		result += ` selected`
	}
	result += `>dashboard.html</option>`

	// index.html
	result += `<option title="index.html" value="index"`
	if template == "index" {
		result += ` selected`
	}
	result += `>index.html</option>`

	// page.html
	result += `<option title="page.html" value="page"`
	if template == "" || template == "page" {
		result += ` selected`
	}
	result += `>page.html</option>`

	// User templates
	if files, err := ioutil.ReadDir(wrap.DTemplate); err == nil {
		for _, file := range files {
			if len(file.Name()) > 0 && file.Name()[0] == '.' {
				continue
			}
			if len(file.Name()) > 0 && strings.ToLower(file.Name()) == "robots.txt" {
				continue
			}
			if !wrap.IsSystemMountedTemplateFile(file.Name()) {
				value := this.dashboard_TemplateNameToValue(file.Name())
				result += `<option title="` + file.Name() + `" value="` + value + `"`
				if template == value {
					result += ` selected`
				}
				result += `>` + file.Name() + `</option>`
			}
		}
	}

	return result
}

func (this *Modules) RegisterModule_Dashboard() *Module {
	return this.newModule(MInfo{
		Mount: "license",
		Name:  "Сервер лицензий",
		Order: 0,
		Icon:  "<i class=\"material-icons notranslate\">dashboard</i>", //assets.SysSvgIconPage,
		Sub: &[]MISub{
			{Mount: "default", Name: "Список панелей", Show: true, Icon: "<i class=\"material-icons notranslate\">list</i>" /*assets.SysSvgIconList*/},
			{Mount: "add", Name: "Добавить панель", Show: true, Icon: "<i class=\"material-icons notranslate\">add</i>" /*assets.SysSvgIconPlus*/},
			{Mount: "modify", Name: "Редактировать", Show: false},
		},
	}, func(wrap *wrapper.Wrapper) {
		// Front-end
		row := &utils.Sql_page{}
		rou := &utils.Sql_user{}
		err := wrap.DB.QueryRow(
			wrap.R.Context(),
			// `SELECT
			// 	dashboards.id,
			// 	dashboards.user,
			// 	dashboards.template,
			// 	dashboards.name,
			// 	dashboards.alias,
			// 	dashboards.content,
			// 	dashboards.meta_title,
			// 	dashboards.meta_keywords,
			// 	dashboards.meta_description,
			// 	UNIX_TIMESTAMP(dashboards.datetime) as datetime,
			// 	dashboards.active,
			// 	users.id,
			// 	users.first_name,
			// 	users.last_name,
			// 	users.email,
			// 	users.admin,
			// 	users.active
			// FROM
			// 	dashboards
			// 	LEFT JOIN users ON users.id = dashboards.user
			// WHERE
			// 	dashboards.active = 1 and
			// 	dashboards.alias = ?
			// LIMIT 1;`,
			`SELECT
				dashboards.id,
				dashboards.user,
				dashboards.template,
				dashboards.name,
				dashboards.alias,
				dashboards.content,
				dashboards.meta_title,
				dashboards.meta_keywords,
				dashboards.meta_description,
				strftime('%s', dashboards.datetime) as datetime,
				dashboards.active,
				users.id,
				users.first_name,
				users.last_name,
				users.email,
				users.admin,
				users.active
			FROM
				dashboards
				LEFT JOIN users ON users.id = dashboards.user
			WHERE
				dashboards.active = 1 and
				dashboards.alias = ?
			LIMIT 1;`,
			wrap.R.URL.Path,
		).Scan(
			&row.A_id,
			&row.A_user,
			&row.A_template,
			&row.A_name,
			&row.A_alias,
			&row.A_content,
			&row.A_meta_title,
			&row.A_meta_keywords,
			&row.A_meta_description,
			&row.A_datetime,
			&row.A_active,
			&rou.A_id,
			&rou.A_first_name,
			&rou.A_last_name,
			&rou.A_email,
			&rou.A_admin,
			&rou.A_active,
		)
		if err != nil && err != wrapper.ErrNoRows {
			// System error 500
			wrap.LogCpError(&err)
			utils.SystemErrorPageEngine(wrap.W, err)
			return
		} else if err == wrapper.ErrNoRows {
			// User error 404 page
			// wrap.RenderFrontEnd("404", fetdata.New(wrap, true, nil, nil), http.StatusNotFound)
			wrap.RenderFrontEnd("dashboard", fetdata.New(wrap, true, row, rou), http.StatusNotFound)
			return
		}

		// Render template
		wrap.RenderFrontEnd(row.A_template, fetdata.New(wrap, false, row, rou), http.StatusOK)
	}, func(wrap *wrapper.Wrapper) (string, string, string) {
		content := ""
		sidebar := ""
		if wrap.CurrSubModule == "" || wrap.CurrSubModule == "default" {
			content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
				{Name: "Список панелей"},
			})
			content += builder.DataTable(
				wrap,
				"dashboards",
				"id",
				"DESC",
				&[]builder.DataTableRow{
					{
						DBField: "id",
					},
					{
						DBField: "template",
					},
					{
						DBField:     "name",
						NameInTable: "Page / URL",
						CallBack: func(values *[]string) string {
							name := `<a href="/cp/` + wrap.CurrModule + `/modify/` + (*values)[0] + `/">` + html.EscapeString((*values)[2]) + `</a>`
							alias := html.EscapeString((*values)[3])
							template := html.EscapeString((*values)[1]) + ".html"
							return `<div>` + name + `</div><div class="template"><small>` + template + `</small></div><div><small>` + alias + `</small></div>`
						},
					},
					{
						DBField: "alias",
					},
					{
						DBField: "datetime",
						//DBExp:       "UNIX_TIMESTAMP(`datetime`)",
						DBExp:       "strftime('%s', datetime)",
						NameInTable: "Date / Time",
						Classes:     "d-none d-md-table-cell",
						CallBack: func(values *[]string) string {
							t := int64(utils.StrToInt((*values)[4]))
							return `<div>` + utils.UnixTimestampToFormat(t, "02.01.2006") + `</div>` +
								`<div><small>` + utils.UnixTimestampToFormat(t, "15:04:05") + `</small></div>`
						},
					},
					{
						DBField:     "active",
						NameInTable: "Active",
						Classes:     "d-none d-sm-table-cell",
						CallBack: func(values *[]string) string {
							return builder.CheckBox(utils.StrToInt((*values)[5]))
						},
					},
				},
				func(values *[]string) string {
					return builder.DataTableAction(&[]builder.DataTableActionRow{
						{
							Icon:   assets.SysSvgIconView,
							Href:   (*values)[3],
							Hint:   "View",
							Target: "_blank",
						},
						{
							Icon: assets.SysSvgIconEdit,
							Href: "/cp/" + wrap.CurrModule + "/modify/" + (*values)[0] + "/",
							Hint: "Edit",
						},
						{
							Icon: assets.SysSvgIconRemove,
							Href: "javascript:fave.ActionDataTableDelete(this,'index-delete','" +
								(*values)[0] + "','Are you sure want to delete page?');",
							Hint:    "Delete",
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
					{Name: "Добавить панель"},
				})
			} else {
				content += this.getBreadCrumbs(wrap, &[]consts.BreadCrumb{
					{Name: "Редактировать панель"},
				})
			}

			data := utils.Sql_page{
				A_id:               0,
				A_user:             0,
				A_template:         "",
				A_name:             "",
				A_alias:            "",
				A_content:          "",
				A_meta_title:       "",
				A_meta_keywords:    "",
				A_meta_description: "",
				A_datetime:         0,
				A_active:           0,
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
						user,
						template,
						name,
						alias,
						content,
						meta_title,
						meta_keywords,
						meta_description,
						active
					FROM
						dashboards
					WHERE
						id = ?
					LIMIT 1;`,
					utils.StrToInt(wrap.UrlArgs[2]),
				).Scan(
					&data.A_id,
					&data.A_user,
					&data.A_template,
					&data.A_name,
					&data.A_alias,
					&data.A_content,
					&data.A_meta_title,
					&data.A_meta_keywords,
					&data.A_meta_description,
					&data.A_active,
				)
				if *wrap.LogCpError(&err) != nil {
					return "", "", ""
				}
			}

			btn_caption := "Добавить"
			if wrap.CurrSubModule == "modify" {
				btn_caption = "Save"
			}

			content += builder.DataForm(wrap, []builder.DataFormField{
				{
					Kind:  builder.DFKHidden,
					Name:  "action",
					Value: "dashboard-modify",
				},
				{
					Kind:  builder.DFKHidden,
					Name:  "id",
					Value: utils.IntToStr(data.A_id),
				},
				{
					Kind:     builder.DFKText,
					Caption:  "Название панели",
					Name:     "name",
					Value:    data.A_name,
					Required: true,
					Min:      "1",
					Max:      "255",
				},
				{
					Kind:    builder.DFKText,
					Caption: "Алиас",
					Name:    "alias",
					Value:   data.A_alias,
					Hint:    "Example: /about-us/ or /about-us.html",
					Max:     "255",
				},
				{
					Kind:    builder.DFKText,
					Caption: "Шаблон панели",
					Name:    "template",
					Value:   "0",
					CallBack: func(field *builder.DataFormField) string {
						return `<div class="form-group n2">` +
							`<div class="row">` +
							`<label for="lbl_template" class="col-sm-3 col-form-label">Шаблон страницы</label>` +
							`<div class="col-sm-9">` +
							`<select class="selectpicker" data-style="select-with-transition" id="lbl_template" name="template" data-live-search="true">` +
							this.dashboard_GetTemplateSelectOptions(wrap, data.A_template) +
							`</select>` +
							`</div>` +
							`</div>` +
							`</div>`
					},
				},
				{
					Kind:    builder.DFKTextArea,
					Caption: "Содержание панели",
					Name:    "content",
					Value:   data.A_content,
					Classes: "wysiwyg",
				},
				{
					Kind:    builder.DFKText,
					Caption: "Оглавление",
					Name:    "meta_title",
					Value:   data.A_meta_title,
					Max:     "255",
				},
				{
					Kind:    builder.DFKText,
					Caption: "Ключевые слова",
					Name:    "meta_keywords",
					Value:   data.A_meta_keywords,
					Max:     "255",
				},
				{
					Kind:    builder.DFKTextArea,
					Caption: "Описание",
					Name:    "meta_description",
					Value:   data.A_meta_description,
					Max:     "510",
				},
				{
					Kind:    builder.DFKCheckBox,
					Caption: "Активность",
					Name:    "active",
					Value:   utils.IntToStr(data.A_active),
				},
				{
					Kind:   builder.DFKSubmit,
					Value:  btn_caption,
					Target: "add-edit-button",
				},
			})

			if wrap.CurrSubModule == "add" {
				sidebar += `<button class="btn btn-primary btn-sidebar" id="add-edit-button">Добавить</button>`
			} else {
				sidebar += `<button class="btn btn-primary btn-sidebar" id="add-edit-button">Сохранить</button>`
			}
		}
		return this.getSidebarModules(wrap), content, sidebar
	})
}
