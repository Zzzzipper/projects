package consts

import (
	"html/template"
)

const AssetsPath = "assets"
const DirIndexFile = "index.html"

// Bootstrap resources
const AssetsBootstrapCss = AssetsPath + "/bootstrap.css"
const AssetsBootstrapJs = AssetsPath + "/bootstrap.js"
const AssetsJqueryJs = AssetsPath + "/jquery.js"
const AssetsPopperJs = AssetsPath + "/popper.js"

// Material design
const AssetsMaterialDashboardCss = AssetsPath + "/material-dashboard.min.css"
const AssetsMaterilaDemoCss = "demo/demo.css"
const AssetsBootstrapMaterialDesign = AssetsPath + "/bootstrap-material-design.min.js"
const AssetsMaterilaDashboardMinJs = AssetsPath + "/material-dashboard.min.js"
const AssetsPerfectScroolbarJs = AssetsPath + "/perfect-scrollbar.jquery.min.js"
const AssetsMomentJs = AssetsPath + "/moment.min.js"
const AssetsSweetalert2Js = AssetsPath + "/sweetalert2.js"
const AssetsValidateJs = AssetsPath + "/jquery.validate.min.js"
const AssetsBootstrapWizardJs = AssetsPath + "/jquery.bootstrap-wizard.js"
const AssetsBootstrapSelectPickerJs = AssetsPath + "/bootstrap-selectpicker.js"
const AssetsBootstrapDatetimePickerJs = AssetsPath + "/bootstrap-datetimepicker.min.js"
const AssetsJqueryDataTablesJs = AssetsPath + "/jquery.dataTables.min.js"
const AssetsBootstrapTagsInputJs = AssetsPath + "/bootstrap-tagsinput.js"
const AssetsJasnyBootstrapJs = AssetsPath + "/jasny-bootstrap.min.js"
const AssetsFullcalendarJs = AssetsPath + "/fullcalendar.min.js"
const AssetsJqueryJvectorMapJs = AssetsPath + "/jquery-jvectormap.js"
const AssetsNouisliderJs = AssetsPath + "/nouislider.min.js"
const AssetsArriveJs = AssetsPath + "/arrive.min.js"
const AssetsChartistJs = AssetsPath + "/chartist.min.js"
const AssetsChartMinCss = AssetsPath + "/chart.min.css"
const AssetsChartMinJs = AssetsPath + "/chart.min.js"
const AssetsUtilsJs = AssetsPath + "/utils.js"
const AssetsBootstrapNotifyJs = AssetsPath + "/bootstrap-notify.js"
const AssetsDemoJs = AssetsPath + "/demo.js"
const AssetsBootstrapMinCss = AssetsPath + "/bootstrap.min.css"
const AssetsImageSidebarJpeg1 = AssetsPath + "/img/sidebar-1.jpg"
const AssetsImageSidebarJpeg2 = AssetsPath + "/img/sidebar-2.jpg"
const AssetsImageSidebarJpeg3 = AssetsPath + "/img/sidebar-3.jpg"
const AssetsImageSidebarJpeg4 = AssetsPath + "/img/sidebar-4.jpg"
const AssetsSuiteJs = AssetsPath + "/codebase/suite.js"
const AssetsSuiteCss = AssetsPath + "/codebase/suite.css"
const AssetsGridCss = AssetsPath + "/codebase/grid.css"
const AssetsDatasetJs = AssetsPath + "/codebase/dataset.js"

// System resources
const AssetsCpImgLoadGif = AssetsPath + "/cp/img-load.gif"
const AssetsCpScriptsJs = AssetsPath + "/cp/scripts.js"
const AssetsCpStylesCss = AssetsPath + "/cp/styles.css"
const AssetsSysBgPng = AssetsPath + "/sys/bg.png"
const AssetsSysFaveIco = AssetsPath + "/sys/fave.ico"
const AssetsSysLogoPng = AssetsPath + "/sys/logo.png"
const AssetsSysLogoSvg = AssetsPath + "/sys/logo.svg"
const AssetsSysStylesCss = AssetsPath + "/sys/styles.css"
const AssetsSysPlaceholderPng = AssetsPath + "/sys/placeholder.png"

// Wysiwyg editor
const AssetsCpWysiwygPellCss = AssetsPath + "/cp/wysiwyg/pell.css"
const AssetsCpWysiwygPellJs = AssetsPath + "/cp/wysiwyg/pell.js"

// CodeMirror template editor
const AssetsCpCodeMirrorCss = AssetsPath + "/cp/tmpl-editor/codemirror.css"
const AssetsCpCodeMirrorJs = AssetsPath + "/cp/tmpl-editor/codemirror.js"

// LightGallery for products
const AssetsLightGalleryCss = AssetsPath + "/lightgallery.css"
const AssetsLightGalleryJs = AssetsPath + "/lightgallery.js"

// Make global for other packages
var ParamDebug bool
var ParamKeepAlive bool
var ParamPort int
var ParamWwwDir string
var ParamGrpcGateHost string
var ParamGrpcGatePort int
var ParamGrpcGateTimeOut int = 30

var HasGrpcGate bool = false
var GuestUserId int32 = 1
var AllowEmptyPassword = true

// 536903680

// For admin panel
type BreadCrumb struct {
	Name string
	Link string
}

// Template data
type TmplSystem struct {
	CpSubModule                   string
	InfoVersion                   string
	PathCssBootstrap              string
	PathCssBootstrapMin           string
	PathCssMaterilaDashboard      string
	PathCssCpCodeMirror           string
	PathCssCpStyles               string
	PathCssCpWysiwygPell          string
	PathCssLightGallery           string
	PathCssStyles                 string
	PathIcoFav                    string
	PathJsBootstrap               string
	PathJsCpCodeMirror            string
	PathJsCpScripts               string
	PathJsCpWysiwygPell           string
	PathJsJquery                  string
	PathJsLightGallery            string
	PathJsPopper                  string
	PathSvgLogo                   string
	PathThemeScripts              string
	PathThemeStyles               string
	PathCssGoogleFonts            string
	PathCssAvesomeFonts           string
	PathMaterialDemoCss           string
	PathMaterialDashboardMinJs    string
	PathBootstrapMaterialDesign   string
	PathPerfectScroolbarJs        string
	PathMomentJs                  string
	PathSweetalert2Js             string
	PathValidateJs                string
	PathBootstrapWizardJs         string
	PathBootstrapSelectPickerJs   string
	PathBootstrapDatetimePickerJs string
	PathJqueryDataTablesJs        string
	PathBootstrapTagsInputJs      string
	PathJasnyBootstrapJs          string
	PathFullcalendarJs            string
	PathJqueryJvectorMapJs        string
	PathNouisliderJs              string
	PathCloudFareJs               string
	PathArriveJs                  string
	PathChartistJs                string
	PathChartMinCss               string
	PathChartMinJs                string
	PathUtilsJs                   string
	PathBootstrapNotifyJs         string
	PathDemoJs                    string
	PathImageSidebarJpeg1         string
	PathImageSidebarJpeg2         string
	PathImageSidebarJpeg3         string
	PathImageSidebarJpeg4         string
	PathSuiteJs                   string
	PathSuiteCss                  string
	PathGridCss                   string
	PathDatasetJs                 string
	CpModule                      string
}

type TmplError struct {
	ErrorMessage string
}

type TmplData struct {
	System  TmplSystem
	Data    interface{}
	Signals interface{}
}

type TmplDataCpBase struct {
	Caption            string
	Content            template.HTML
	ModuleCurrentAlias string
	NavBarModules      template.HTML
	NavBarModulesSys   template.HTML
	SidebarLeft        template.HTML
	SidebarRight       template.HTML
	Title              string
	UserAvatarLink     string
	UserEmail          string
	UserFirstName      string
	UserId             int
	UserLastName       string
	UserPassword       string
	ServerAddress      string
	ServerPort         int
	BodyClasses        string
}

type TmplOrderClient struct {
	LastName        string
	FirstName       string
	MiddleName      string
	Phone           string
	Email           string
	DeliveryComment string
	OrderComment    string
}

type TmplOrderElse struct {
	OrderId     int64
	Subject     string
	CpOrderLink string
}

type TmplEmailOrder struct {
	Basket interface{}
	Client TmplOrderClient
	Else   TmplOrderElse
}
