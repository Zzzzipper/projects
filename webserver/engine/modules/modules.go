package modules

import (
	"fmt"
	"html"
	"reflect"
	"sort"
	"strings"
	"time"

	"server/engine/assets"
	"server/engine/consts"
	"server/engine/wrapper"
)

type MISub struct {
	Mount string
	Name  string
	Icon  string
	Show  bool
	Sep   bool
}

type MInfo struct {
	Id     string
	Mount  string
	Name   string
	Order  int
	System bool
	Icon   string
	Sub    *[]MISub
}

type Module struct {
	Info  MInfo
	Front func(wrap *wrapper.Wrapper)
	Back  func(wrap *wrapper.Wrapper) (string, string, string)
}

type AInfo struct {
	Id        string
	Mount     string
	WantUser  bool
	WantAdmin bool
}

type Action struct {
	Info AInfo
	Act  func(wrap *wrapper.Wrapper)
}

type DInfo struct {
	Id    string
	Mount string
}

type Data struct {
	Info DInfo
	Post func(wrap *wrapper.Wrapper)
}

type Modules struct {
	mods map[string]*Module
	acts map[string]*Action
	data map[string]*Data
}

func (this *Modules) load() {
	t := reflect.TypeOf(this)
	for i := 0; i < t.NumMethod(); i++ {
		m := t.Method(i)
		if strings.HasPrefix(m.Name, "XXX") {
			continue
		}
		if strings.HasPrefix(m.Name, "RegisterModule_") {
			id := m.Name[15:]
			if _, ok := reflect.TypeOf(this).MethodByName("RegisterModule_" + id); ok {
				result := reflect.ValueOf(this).MethodByName("RegisterModule_" + id).Call([]reflect.Value{})
				if len(result) >= 1 {
					mod := result[0].Interface().(*Module)
					mod.Info.Id = id
					this.mods[mod.Info.Mount] = mod
				}
			}
		}
		if strings.HasPrefix(m.Name, "RegisterAction_") {
			id := m.Name[15:]
			if _, ok := reflect.TypeOf(this).MethodByName("RegisterAction_" + id); ok {
				result := reflect.ValueOf(this).MethodByName("RegisterAction_" + id).Call([]reflect.Value{})
				if len(result) >= 1 {
					act := result[0].Interface().(*Action)
					act.Info.Id = id
					this.acts[act.Info.Mount] = act
				}
			}
		}
		if strings.HasPrefix(m.Name, "RegisterData_") {
			id := m.Name[13:]
			if _, ok := reflect.TypeOf(this).MethodByName("RegisterData_" + id); ok {
				result := reflect.ValueOf(this).MethodByName("RegisterData_" + id).Call([]reflect.Value{})
				if len(result) >= 1 {
					data := result[0].Interface().(*Data)
					data.Info.Id = id
					this.data[data.Info.Mount] = data
				}
			}
		}
	}
}

func (this *Modules) newModule(info MInfo, ff func(wrap *wrapper.Wrapper), bf func(wrap *wrapper.Wrapper) (string, string, string)) *Module {
	return &Module{Info: info, Front: ff, Back: bf}
}

func (this *Modules) newAction(info AInfo, af func(wrap *wrapper.Wrapper)) *Action {
	return &Action{Info: info, Act: af}
}

func (this *Modules) newData(info DInfo, df func(wrap *wrapper.Wrapper)) *Data {
	return &Data{Info: info, Post: df}
}

func (this *Modules) getCurrentModule(wrap *wrapper.Wrapper, backend bool) (*Module, string) {
	var mod *Module = nil
	var modCurr string = ""

	// Some module
	if len(wrap.UrlArgs) >= 1 {
		if m, ok := this.mods[wrap.UrlArgs[0]]; ok {
			if (!backend && m.Front != nil) || (backend && m.Back != nil) {
				mod = m
				modCurr = wrap.UrlArgs[0]
			}
		}
	}

	// Default module
	if !backend || (backend && len(wrap.UrlArgs) <= 0) {
		if mod == nil {
			if m, ok := this.mods["index"]; ok {
				mod = m
				modCurr = "index"
			}
		}
	}

	// Selected module
	// if !backend {
	// 	if len(wrap.UrlArgs) <= 0 {
	// 		if (*wrap.Config).Engine.MainModule > 0 {
	// 			if (*wrap.Config).Engine.MainModule == 1 {
	// 				if m, ok := this.mods["dashboard"]; ok {
	// 					mod = m
	// 					modCurr = "dashboard"
	// 				}
	// 			} else if (*wrap.Config).Engine.MainModule == 2 {
	// 				if m, ok := this.mods["shop"]; ok {
	// 					mod = m
	// 					modCurr = "shop"
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	return mod, modCurr
}

func (this *Modules) getModulesList(wrap *wrapper.Wrapper, sys bool, all bool) []*MInfo {
	list := make([]*MInfo, 0)
	for _, mod := range this.mods {
		if mod.Back != nil {
			if mod.Info.System == sys || all {
				list = append(list, &mod.Info)
			}
		}
	}
	sort.Slice(list, func(i, j int) bool {
		return list[i].Order < list[j].Order
	})
	return list
}

func (this *Modules) getSidebarModuleSubMenu(wrap *wrapper.Wrapper, mod *MInfo) string {
	html := ``
	collapsed := true
	containerClass := `collapse`
	if mod.Sub != nil {
		for _, item := range *mod.Sub {
			if item.Show {
				if !item.Sep {

					href := "/cp/" + mod.Mount + "/" + item.Mount + "/"
					thisIsTarget := targetEqualPresent(wrap.R.RequestURI, href)
					if thisIsTarget && collapsed {
						collapsed = !thisIsTarget
					}

					class := ""
					if /*item.Mount == "default" && len(wrap.UrlArgs) <= 1 ||
					(len(wrap.UrlArgs) >= 2 && item.Mount == wrap.UrlArgs[1]) ||*/
					(len(wrap.UrlArgs) >= 2 && item.Mount == "default" && wrap.UrlArgs[1] == "modify") ||
						(len(wrap.UrlArgs) >= 2 && len(strings.Split(item.Mount, "-")) <= 1 &&
							len(strings.Split(wrap.UrlArgs[1], "-")) >= 2 &&
							strings.Split(wrap.UrlArgs[1], "-")[1] == "modify" &&
							strings.Split(item.Mount, "-")[0] == strings.Split(wrap.UrlArgs[1], "-")[0]) ||
						thisIsTarget == true {
						class = " active"
					}

					icon := item.Icon
					if icon == "" {
						icon = assets.SysSvgIconGear
					}
					if mod.Mount == "index" && item.Mount == "default" {
						href = "/cp/"
					} else if item.Mount == "default" {
						href = "/cp/" + mod.Mount + "/"
					}

					html += `<li class="nav-item` + class + `"><a class="nav-link" href="` + href + `">` + icon + `<p>` + item.Name + `</p>` + `</a></li>`

				} else {
					html += `<li class="nav-separator"></li>`
				}
			}
		}

		if collapsed == false {
			containerClass += " show"
		}

		if html != "" {
			// html = `<ul class="nav flex-column">` + html + `</ul>`
			html = `<div class="` + containerClass + `" id="pages-` + mod.Mount + `"><ul class="nav flex-column">` + html + `</ul><div>`
		}
	}
	return html
}

func (this *Modules) getNavMenuModules(wrap *wrapper.Wrapper, sys bool) string {
	html := ``
	list := this.getModulesList(wrap, sys, false)
	for _, mod := range list {
		class := ""
		if mod.Mount == wrap.CurrModule {
			class += " active"
		}
		// if mod.Mount == "blog" && (*wrap.Config).Modules.Enabled.Blog == 0 {
		// 	class += " disabled"
		// } /*else if mod.Mount == "shop" && (*wrap.Config).Modules.Enabled.Shop == 0 {
		// 	class += " disabled"
		// }*/
		href := `/cp/` + mod.Mount + `/`
		if mod.Mount == "index" {
			href = `/cp/`
		}
		if !(sys && (mod.Mount == "api")) {
			html += `<a class="dropdown-item` + class + `" href="` + href + `">` + `<p>` + mod.Name + `</p>` + `</a>`
		}
	}
	return html
}

func targetEqualPresent(uri string, path string) bool {
	return uri == strings.Replace(path, "default/", "", -1) ||
		uri == strings.Replace(path, "index/default/", "", -1)
}

func (this *Modules) isSubmenuHaveActiveItem(wrap *wrapper.Wrapper, root *MInfo) *MISub {
	if root == nil {
		return nil
	}
	if root.Sub != nil {
		for _, item := range *root.Sub {
			href := `/cp/` + root.Mount + `/` + item.Mount + `/`
			if targetEqualPresent(wrap.R.RequestURI, href) {
				return &item
			}
		}
	}
	return nil
}

func (this *Modules) getSidebarModules(wrap *wrapper.Wrapper) string {
	html_def := ""
	html_sys := ""
	list := this.getModulesList(wrap, false, true)
	for _, mod := range list {
		class := ""
		submenu := ""
		state := "aria-expanded=false"
		submenu = this.getSidebarModuleSubMenu(wrap, mod)
		if mod.Mount == wrap.CurrModule {
			class += " active"
			if this.isSubmenuHaveActiveItem(wrap, mod) != nil {
				state = "aria-expanded=true"
			}
		}
		// if mod.Mount == "blog" && (*wrap.Config).Modules.Enabled.Blog == 0 {
		// 	class += " disabled"
		// } /* else if mod.Mount == "shop" && (*wrap.Config).Modules.Enabled.Shop == 0 {
		// 	class += " disabled"
		// }*/
		icon := mod.Icon
		if icon == "" {
			icon = assets.SysSvgIconGear
		}
		//href := "/cp/" + mod.Mount + "/"
		//if mod.Mount == "index" {
		//	href = "/cp/"
		//}
		if !mod.System {
			// data-toggle="collapse" href="#pagesExamples" aria-expanded="true"
			// html_def += `<li class="nav-item` + class + `"><a class="nav-link" href="` + href + `">` + icon + `<p>` + mod.Name + `</p>` + `</a>` + submenu + `</li>`
			html_def += `<li class="nav-item` + class + `"><a class="nav-link" data-toggle="collapse" href="#pages-` + mod.Mount + `" ` + state + `>` + icon + `<p>` + mod.Name + `<b class="caret"></b></p>` + `</a>` + submenu + `</li>`
		} else {
			if !(mod.Mount == "api") {
				// html_sys += `<li class="nav-item` + class + `"><a class="nav-link" href="` + href + `">` + icon + `<p>` + mod.Name + `</p>` + `</a>` + submenu + `</li>`
				html_sys += `<li class="nav-item` + class + `"><a class="nav-link" data-toggle="collapse" href="#pages-` + mod.Mount + `" ` + state + `>` + icon + `<p>` + mod.Name + `<b class="caret"></b></p>` + `</a>` + submenu + `</li>`
			}
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
	return html_def + html_sys
}

func (this *Modules) getBreadCrumbs(wrap *wrapper.Wrapper, data *[]consts.BreadCrumb) string {
	res := `<nav aria-label="breadcrumb">`
	res += `<ol class="breadcrumb">`
	if this.mods[wrap.CurrModule].Info.Mount == "index" {
		res += `<li class="breadcrumb-item"><a href="/cp/">` + html.EscapeString(this.mods[wrap.CurrModule].Info.Name) + `</a></li>`
	} else {
		res += `<li class="breadcrumb-item"><a href="/cp/` + this.mods[wrap.CurrModule].Info.Mount + `/">` + html.EscapeString(this.mods[wrap.CurrModule].Info.Name) + `</a></li>`
	}
	for _, item := range *data {
		if item.Link == "" {
			res += `<li class="breadcrumb-item active" aria-current="page">` + html.EscapeString(item.Name) + `</li>`
		} else {
			res += `<li class="breadcrumb-item"><a href="` + item.Link + `">` + html.EscapeString(item.Name) + `</a></li>`
		}
	}
	res += `</ol>`
	res += `</nav>`
	return res
}

func New() *Modules {
	m := Modules{
		mods: map[string]*Module{},
		acts: map[string]*Action{},
		data: map[string]*Data{},
	}
	m.load()
	return &m
}

func (this *Modules) XXXActionHeaders(wrap *wrapper.Wrapper, status int) {
	wrap.W.WriteHeader(status)
	wrap.W.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
	wrap.W.Header().Set("Content-Type", "text/html; charset=utf-8")
}

func (this *Modules) XXXActionFire(wrap *wrapper.Wrapper) bool {
	if wrap.R.Method == "POST" {
		if err := wrap.R.ParseForm(); err == nil {
			name := wrap.R.FormValue("action")
			if name == "" {
				wrap.R.ParseMultipartForm(32 << 20)
				name = wrap.R.FormValue("action")
			}
			// if name != "" {
			// 	if act, ok := this.acts[name]; ok {
			// 		// Check for MySQL connection
			// 		if name != "index-mysql-setup" {
			// 			if err := wrap.UseDatabase(); err != nil {
			// 				this.XXXActionHeaders(wrap, http.StatusNotFound)
			// 				wrap.MsgError(err.Error())
			// 				return true
			// 			}
			// 			if act.Info.WantUser || act.Info.WantAdmin {
			// 				if !wrap.LoadSessionUser() {
			// 					this.XXXActionHeaders(wrap, http.StatusNotFound)
			// 					wrap.MsgError(`Вы должны быть авторизованы для выполнения этой операции`)
			// 					return true
			// 				}
			// 				if wrap.User.A_active <= 0 {
			// 					if !wrap.LoadSessionUser() {
			// 						this.XXXActionHeaders(wrap, http.StatusNotFound)
			// 						wrap.MsgError(`Вы не имеете достаточных прав для выполнения этой операции`)
			// 						return true
			// 					}
			// 				}
			// 			}
			// 			if act.Info.WantAdmin && wrap.User.A_admin <= 0 {
			// 				if !wrap.LoadSessionUser() {
			// 					this.XXXActionHeaders(wrap, http.StatusNotFound)
			// 					wrap.MsgError(`Вы не имеете достаточных прав для выполнения этой операции`)
			// 					return true
			// 				}
			// 			}
			// 		}
			// 		this.XXXActionHeaders(wrap, http.StatusOK)
			// 		act.Act(wrap)
			// 		return true
			// 	} else {
			// 		this.XXXActionHeaders(wrap, http.StatusNotFound)
			// 		wrap.MsgError(`Эта операция не реализована`)
			// 		return true
			// 	}
			// }
		}
	}
	return false
}

func (this *Modules) XXXRequestDataFire(wrap *wrapper.Wrapper) bool {
	if wrap.R.Method == "POST" || wrap.R.Method == "OPTIONS" {
		if len(wrap.UrlArgs) >= 1 {
			var key string = wrap.UrlArgs[0]
			for i := 1; i < len(wrap.UrlArgs); i++ {
				key += "-" + wrap.UrlArgs[i]
			}
			//fmt.Println("XXXRequestDataFire key", key)
			if m, ok := this.data[key]; ok {
				m.Post(wrap)
				return true
			}
		}
	}
	return false
}

func (this *Modules) XXXFrontEnd(wrap *wrapper.Wrapper) bool {
	mod, cm := this.getCurrentModule(wrap, false)
	if mod == nil {
		return false
	}

	// Index module, if module disabled
	// if mod.Info.Mount == "blog" || mod.Info.Mount == "shop" {
	// 	if (*wrap.Config).Modules.Enabled.Blog == 0 /*|| (*wrap.Config).Modules.Enabled.Shop == 0*/ {
	// 		if m, ok := this.mods["index"]; ok {
	// 			if mod.Info.Mount == "blog" && (*wrap.Config).Modules.Enabled.Blog == 0 {
	// 				mod = m
	// 				cm = "index"
	// 			} /* else if mod.Info.Mount == "shop" && (*wrap.Config).Modules.Enabled.Shop == 0 {
	// 				mod = m
	// 				cm = "index"
	// 			}*/
	// 		}
	// 	}
	// }

	if mod.Info.Mount == "dashboard" && len(wrap.UrlArgs) >= 2 && wrap.UrlArgs[1] != "" {
		// Текущий дашборд - ссылка на документ или комнаду
		wrap.CurrDashboard = wrap.UrlArgs[len(wrap.UrlArgs)-1]
	}

	wrap.CurrModule = cm
	if mod.Front != nil {
		start := time.Now()
		mod.Front(wrap)
		if !(mod.Info.Mount == "api" || (mod.Info.Mount == "shop" && len(wrap.UrlArgs) >= 3 && wrap.UrlArgs[1] == "basket")) {
			wrap.W.Write([]byte(fmt.Sprintf("<!-- %.3f ms -->", time.Now().Sub(start).Seconds())))
		}
		return true
	}

	return false
}

func (this *Modules) XXXBackEnd(wrap *wrapper.Wrapper) bool {
	mod, cm := this.getCurrentModule(wrap, true)
	if mod != nil {
		wrap.CurrModule = cm
		if len(wrap.UrlArgs) >= 2 && wrap.UrlArgs[1] != "" {
			wrap.CurrSubModule = wrap.UrlArgs[1]
		}

		// Search for sub module mount
		found := false
		submount := "default"
		if wrap.CurrSubModule != "" {
			submount = wrap.CurrSubModule
		}
		for _, item := range *mod.Info.Sub {
			if item.Mount == submount {
				found = true
				break
			}
		}

		// Display standart 404 error page
		if !found {
			return found
		}

		// Call module function
		if mod.Back != nil {
			sidebar_left, content, sidebar_right := mod.Back(wrap)

			// Display standart 404 error page
			if sidebar_left == "" && content == "" && sidebar_right == "" {
				return false
			}

			// Prepare CP page
			body_class := "cp"
			if sidebar_left != "" {
				body_class = body_class + " cp-sidebar-left"
			}
			if content == "" {
				body_class = body_class + " cp-404"
			}
			if sidebar_right != "" {
				body_class = body_class + " cp-sidebar-right"
			}

			// wrap.RenderBackEnd(assets.TmplCpBase, consts.TmplDataCpBase{
			// 	Title:              wrap.CurrHost + " - Kotmi WEB " + consts.ServerVersion,
			// 	Caption:            "ver." + consts.ServerVersion,
			// 	BodyClasses:        body_class,
			// 	UserId:             wrap.User.A_id,
			// 	UserFirstName:      utils.JavaScriptVarValue(wrap.User.A_first_name),
			// 	UserLastName:       utils.JavaScriptVarValue(wrap.User.A_last_name),
			// 	UserEmail:          utils.JavaScriptVarValue(wrap.User.A_email),
			// 	ServerAddress:      utils.JavaScriptVarValue(wrap.User.A_email),
			// 	UserAvatarLink:     "https://s.gravatar.com/avatar/" + utils.GetMd5(wrap.User.A_email) + "?s=80&r=g",
			// 	NavBarModules:      template.HTML(this.getNavMenuModules(wrap, false)),
			// 	NavBarModulesSys:   template.HTML(this.getNavMenuModules(wrap, true)),
			// 	ModuleCurrentAlias: wrap.CurrModule,
			// 	SidebarLeft:        template.HTML(sidebar_left),
			// 	Content:            template.HTML(content),
			// 	SidebarRight:       template.HTML(sidebar_right),
			// })

			return true
		}
	}
	return false
}
