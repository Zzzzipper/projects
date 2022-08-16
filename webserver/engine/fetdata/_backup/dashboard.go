package fetdata

import (
	"encoding/json"
	"fmt"
	"html/template"
	"net/url"
	"strings"
	"time"

	"server/engine/gates"
	"server/engine/utils"
	"server/engine/wrapper"
)

type File struct {
	Caption     string `json:"caption"`
	Command     string `json:"command"`
	Ico         string `json:"ico"`
	OneInstance string `json:"oneInstance"`
}

type Folder struct {
	Caption string `json:"caption"`
	Files   []File `json:"files"`
}

type Folders struct {
	Folder []Folder `json:"folder"`
}

type Menu struct {
	Menu interface{} `json:"menu"`
}

type ColumnDef struct {
	alias string
	name  string
	show  bool
}

var eventColumns = []*ColumnDef{
	&ColumnDef{"clnGroup", "Группа", false},
	&ColumnDef{"clnCodeName", "Наименование", true},
	&ColumnDef{"clnCode", "Код", false},
	&ColumnDef{"clnTime", "Время верхнего уровня", true},
	&ColumnDef{"clnDtcp", "Время нижнего уровня", false},
	&ColumnDef{"clnObjName", "Объект", false},
	&ColumnDef{"clnCaption", "Текст", true},
	&ColumnDef{"clnComment", "Примечание", false},
	&ColumnDef{"clnUser", "Пользователь", false},
	&ColumnDef{"clnEquip", "Оборудование", false},
	&ColumnDef{"clnId", "Идентификатор колонки", false},
	&ColumnDef{"clnAcknowledgeState", "Квитирование", true},
}

type Dashboard struct {
	wrap   *wrapper.Wrapper
	object *utils.Sql_page

	user *User
}

type Dashboard_Module struct {
	key       string
	Content   func(d *Dashboard) template.HTML
	Name      func(d *Dashboard) string
	MetaTitle func(d *Dashboard) string
}

var modules = []Dashboard_Module{
	Dashboard_Module{"rts_", RetroContent, RetroName, RetroMetaTitle},
	Dashboard_Module{"scada", InfraContent, InfraName, InfraMetaTitle},
	Dashboard_Module{"evn_", EventContent, EventName, EventMetaTitle},
}

func (this *Dashboard) getModule() *Dashboard_Module {
	for _, m := range modules {
		if strings.Contains(this.wrap.CurrDashboard, m.key) {
			return &m
		}
	}
	return nil
}

func (this *Dashboard) load() *Dashboard {
	return this
}

func (this *Dashboard) Id() int {
	// if this == nil {
	// 	return 0
	// }
	return this.object.A_id
}

func (this *Dashboard) GetLocalPages(menu *Menu) interface{} {
	if menu == nil {
		return nil
	}
	// Читаем все страницы дашбордов и ищем новые
	rows, err := this.wrap.DB.Query(
		this.wrap.R.Context(),
		// `SELECT
		// 		dashboards.name,
		// 		dashboards.alias,
		// 		dashboards.meta_title,
		// 		dashboards.meta_keywords,
		// 		dashboards.meta_description,
		// 		UNIX_TIMESTAMP(dashboards.datetime) as datetime,
		// 		dashboards.active
		// 	FROM
		// 		dashboards
		// 	WHERE
		// 		dashboards.active = 1 and
		// 		dashboards.id = ?;`,
		`SELECT
			dashboards.name,
			dashboards.alias,
			dashboards.meta_title,
			dashboards.meta_keywords,
			dashboards.meta_description,
			strftime('%s', dashboards.datetime) as datetime,
			dashboards.active
		FROM
			dashboards
		WHERE
			dashboards.active = 1 and
			dashboards.id = ?;`,
		this.object.A_id)

	if err != nil {
		fmt.Errorf("error reading local pages: %s\n", err.Error())
		return nil
	}

	if rows != nil {
		row := &utils.Sql_page{}
		for rows.Next() {
			rows.Scan(
				&row.A_name,
				&row.A_alias,
				&row.A_meta_title,
				&row.A_meta_keywords,
				&row.A_meta_description,
				&row.A_datetime,
				&row.A_active,
			)
			// Парсим URL на разделы
			parts := utils.UrlToArray(row.A_alias)
			if menu.Menu == nil {
				menu.Menu = make(map[string]interface{}, 1)
				menu.Menu.(map[string]interface{})["folder"] = make([]interface{}, 0)
			}
			if len(parts) > 1 {
				folderPresent := false
				var dir map[string]interface{}
				for i := range menu.Menu.(map[string]interface{})["folder"].([]interface{}) {
					dir = menu.Menu.(map[string]interface{})["folder"].([]interface{})[i].(map[string]interface{})
					if dir["caption"].(string) == parts[1] {
						folderPresent = true
						break
					}
				}
				if !folderPresent {
					newDir := make(map[string]interface{})
					newDir["caption"] = parts[1]
					dir = newDir
				}
				if len(parts) > 2 {
					filePresent := false
					var f map[string]interface{}
					if dir["file"] != nil {
						for _, file := range dir["file"].([]interface{}) {
							f = file.(map[string]interface{})
							if f["command"].(string) == parts[2] {
								filePresent = true
								break
							}
						}
					} else {
						dir["file"] = make([]interface{}, 0)
					}
					if !filePresent {
						newF := make(map[string]interface{})
						newF["caption"] = row.A_name
						dir["file"] = append(dir["file"].([]interface{}), newF)
						f = newF
					}
					f["command"] = parts[2]
					f["ico"] = "circuit.ico"
					f["oneInstance"] = false
				}
				if !folderPresent {
					menu.Menu.(map[string]interface{})["folder"] = append(menu.Menu.(map[string]interface{})["folder"].([]interface{}), dir)
				}
			}
		}
	}

	return nil

}

func (this *Dashboard) LeftSideBar() template.HTML {
	var html_def string = ""
	var html_sys string = ""

	var jsonString string = ""
	var err error = nil
	// TODO: переделать с учетом запроса с указанием шлюза
	var gate gates.IGate = this.wrap.GatePool.Get("scdgate", this.wrap.S.GetInt("UserId", -1)).(gates.IGate)
	if gate != nil {
		jsonString, err = gate.(gates.IGate).Menu()
		if err != nil {
			fmt.Println(err.Error())
			return ""
		}
	}

	jsonString = strings.Replace(jsonString, "\\", "/", -1)

	if jsonString == "" {
		jsonString = "{}"
	}

	var scadaMenu Menu
	err = json.Unmarshal([]byte(jsonString), &scadaMenu)
	if err != nil {
		fmt.Println(err.Error())
		return ""
	}

	this.GetLocalPages(&scadaMenu)

	if scadaMenu.Menu != nil {
		for _, folder := range scadaMenu.Menu.(map[string]interface{})["folder"].([]interface{}) {
			var icon string = `<i class="material-icons notranslate">folder</i>`
			var submenu string = ""
			var collapsed bool = true
			var containerClass string = `collapse`
			var html string = ``
			state := "aria-expanded=false"
			caption := folder.(map[string]interface{})["caption"].(string)
			for _, o := range folder.(map[string]interface{})["file"].([]interface{}) {
				// Links
				var class string = ""
				href := "/dashboard/" + caption + "/" + o.(map[string]interface{})["command"].(string)
				urlDecoded, _ := url.QueryUnescape(this.wrap.R.RequestURI)
				if urlDecoded == href && collapsed != false {
					state = "aria-expanded=true"
					collapsed = false
					class = " active"
				}
				subItemIcon := `<i class="material-icons notranslate">receipt</i>`
				html += `<li class="nav-item` + class + `"><a class="nav-link" href="` + href + `">` + subItemIcon +
					`<p style="padding-left: 42px; white-space: normal!important; line-height: 1.3em;">` +
					o.(map[string]interface{})["caption"].(string) + `</p>` + `</a></li>`
			}
			if collapsed == false {
				containerClass += " show"
			}
			if html != "" {
				submenu = `<div class="` + containerClass + `" id="pages-` + caption + `"><ul class="nav flex-column">` + html + `</ul><div>`
				html = ""
			}
			html_def += `<li class="nav-item"><a class="nav-link" data-toggle="collapse" href="#pages-` + caption + `" ` + state + `>` + icon + `<p>` + caption + `<b class="caret"></b></p>` + `</a>` + submenu + `</li>`
			submenu = ""
		}
	}

	if html_def != "" {
		html_def = `<ul class="nav flex-column">` + html_def + `</ul>`
	}

	if html_sys != "" {
		html_sys = `<ul class="nav flex-column">` + html_sys + `</ul>`
	}

	if html_def != "" && html_sys != "" {
		html_sys = `<div class="dropdown-divider"></div>` + html_sys
	}

	return template.HTML(html_def + html_sys)
}

func (this *Dashboard) User() *User {
	// if this == nil {
	// 	return nil
	// }
	if this.user != nil {
		return this.user
	}
	this.user = &User{wrap: this.wrap}
	this.user.loadById(this.object.A_user)
	return this.user
}

func (this *Dashboard) Name() string {
	// if this == nil {
	// 	return ""
	// }
	module := this.getModule()
	if module != nil {
		return module.Name(this)
	}
	return this.object.A_name
}

func (this *Dashboard) Alias() string {
	// if this == nil {
	// 	return ""
	// }
	return this.object.A_alias
}

func (this *Dashboard) Content() template.HTML {
	// if this == nil {
	// 	return template.HTML("")
	// }

	module := this.getModule()
	if module != nil {
		return module.Content(this)
	}

	return template.HTML("")
}

func (this *Dashboard) MetaTitle() string {
	// if this == nil {
	// 	return ""
	// }

	module := this.getModule()
	if module != nil {
		return module.MetaTitle(this)
	}
	return this.object.A_meta_title
}

func (this *Dashboard) MetaKeywords() string {
	// if this == nil {
	// 	return ""
	// }
	return this.object.A_meta_keywords
}

func (this *Dashboard) MetaDescription() string {
	// if this == nil {
	// 	return ""
	// }
	return this.object.A_meta_description
}

func (this *Dashboard) DateTimeUnix() int {
	// if this == nil {
	// 	return 0
	// }
	return this.object.A_datetime
}

func (this *Dashboard) DateTimeFormat(format string) string {
	// if this == nil {
	// 	return ""
	// }
	return time.Unix(int64(this.object.A_datetime), 0).Format(format)
}

func (this *Dashboard) Active() bool {
	// if this == nil {
	// 	return false
	// }
	return this.object.A_active > 0
}
