package fetdata

import (
	"bytes"
	"fmt"
	"html"
	"html/template"
	"os"
	"strconv"
	"time"

	"server/engine/consts"
	"server/engine/utils"
	"server/engine/wrapper"
)

type FERData struct {
	wrap  *wrapper.Wrapper
	is404 bool

	Page      *Page
	Dashboard *Dashboard
}

func New(wrap *wrapper.Wrapper, is404 bool, drow interface{}, duser *utils.Sql_user) *FERData {
	var d_Page *Page
	var d_Dashboard *Dashboard

	var preUser *User
	if duser != nil {
		preUser = &User{wrap: wrap, object: duser}
	}

	if wrap.CurrModule == "index" {
		if o, ok := drow.(*utils.Sql_page); ok {
			d_Page = &Page{wrap: wrap, object: o, user: preUser}
		}
	} else if wrap.CurrModule == "dashboard" {
		if o, ok := drow.(*utils.Sql_page); ok {
			d_Dashboard = &Dashboard{wrap: wrap, object: o, user: preUser}
		}
	}

	fer := &FERData{
		wrap:      wrap,
		is404:     is404,
		Page:      d_Page,
		Dashboard: d_Dashboard,
	}

	return fer
}

func (this *FERData) RequestURI() string {
	return this.wrap.R.RequestURI
}

func (this *FERData) RequestURL() string {
	return this.wrap.R.URL.Path
}

func (this *FERData) RequestGET() string {
	return utils.ExtractGetParams(this.wrap.R.RequestURI)
}

func (this *FERData) IsUserLoggedIn() bool {
	if this.wrap.User == nil {
		this.wrap.LoadSessionUser()
	}
	return this.wrap.User != nil && this.wrap.User.A_id > 0
}

func (this *FERData) CurrentUser() *User {
	if this.wrap.User == nil {
		return &User{wrap: this.wrap}
	} else {
		return &User{wrap: this.wrap, object: this.wrap.User}
	}
}

func (this *FERData) Module() string {
	if this.is404 {
		return "404"
	}
	var mod string
	if this.wrap.CurrModule == "index" {
		mod = "index"
	}

	return mod
}

func (this *FERData) UserFirstName() string {
	return utils.JavaScriptVarValue(this.wrap.User.A_first_name)
}

func (this *FERData) UserLastName() string {
	return utils.JavaScriptVarValue(this.wrap.User.A_last_name)
}

func (this *FERData) UserEmail() string {
	//return utils.JavaScriptVarValue(this.wrap.User.A_email)
	return utils.JavaScriptVarValue("TODO: надо исправить")
}

func (this *FERData) ServerAddress() string {
	return utils.JavaScriptVarValue(this.wrap.User.A_address)
}

func (this *FERData) ServerPort() string {
	return utils.JavaScriptVarValue(strconv.Itoa(int(this.wrap.User.A_port)))
}

func (this *FERData) ModuleBlogEnabled() bool {
	return (*this.wrap.Config).Modules.Enabled.Blog != 0
}

func (this *FERData) ModuleShopEnabled() bool {
	return false //(*this.wrap.Config).Modules.Enabled.Shop != 0
}

func (this *FERData) DateTimeUnix() int {
	return int(time.Now().Unix())
}

func (this *FERData) DateTimeFormat(format string) string {
	return time.Unix(int64(time.Now().Unix()), 0).Format(format)
}

func (this *FERData) EscapeString(str string) string {
	return html.EscapeString(str)
}

func (this *FERData) cachedBlock(block string) template.HTML {
	tmpl, err := template.New(block + ".html").Funcs(utils.TemplateAdditionalFuncs()).ParseFiles(
		this.wrap.DTemplate + string(os.PathSeparator) + block + ".html",
	)
	if err != nil {
		return template.HTML(err.Error())
	}
	var tpl bytes.Buffer
	err = tmpl.Execute(&tpl, consts.TmplData{
		System: utils.GetTmplSystemData("", ""),
		Data:   this,
	})
	if err != nil {
		return template.HTML(err.Error())
	}
	return template.HTML(string(tpl.Bytes()))
}

func (this *FERData) CachedBlock1() template.HTML {
	if data, ok := this.wrap.GetBlock1(); ok {
		return data
	}
	data := this.cachedBlock("cached-block-1")
	this.wrap.SetBlock1(data)
	return data
}

func (this *FERData) CachedBlock2() template.HTML {
	if data, ok := this.wrap.GetBlock2(); ok {
		return data
	}
	data := this.cachedBlock("cached-block-2")
	this.wrap.SetBlock2(data)
	return data
}

func (this *FERData) CachedBlock3() template.HTML {
	if data, ok := this.wrap.GetBlock3(); ok {
		fmt.Println("CachedBlock3: " + data)
		return data
	}
	data := this.cachedBlock("cached-block-3")
	this.wrap.SetBlock3(data)
	return data
}

func (this *FERData) CachedBlock4() template.HTML {
	if data, ok := this.wrap.GetBlock4(); ok {
		return data
	}
	data := this.cachedBlock("cached-block-4")
	this.wrap.SetBlock4(data)
	return data
}

func (this *FERData) CachedBlock5() template.HTML {
	if data, ok := this.wrap.GetBlock5(); ok {
		return data
	}
	data := this.cachedBlock("cached-block-5")
	this.wrap.SetBlock5(data)
	return data
}

func (this *FERData) ImagePlaceholderHref() string {
	return utils.GetImagePlaceholderSrc()
}

// func (this *FERData) ShopOrderRequiredLastName() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.LastName != 0
// }

// func (this *FERData) ShopOrderRequiredFirstName() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.FirstName != 0
// }

// func (this *FERData) ShopOrderRequiredMiddleName() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.MiddleName != 0
// }

// func (this *FERData) ShopOrderRequiredMobilePhone() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.MobilePhone != 0
// }

// func (this *FERData) ShopOrderRequiredEmailAddress() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.EmailAddress != 0
// }

// func (this *FERData) ShopOrderRequiredDelivery() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.Delivery != 0
// }

// func (this *FERData) ShopOrderRequiredComment() bool {
// 	return (*this.wrap.Config).Shop.Orders.RequiredFields.Comment != 0
// }
