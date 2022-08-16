package assets

import (
	"fmt"
	"io/ioutil"
	"os"
	"server/engine/assets/codebase"
	"server/engine/assets/css"
	"server/engine/assets/demo"
	"server/engine/assets/img"
	"server/engine/assets/js"
	"server/engine/assets/js/core"
	"server/engine/assets/js/plugins"
	"server/engine/consts"

	"github.com/vladimirok5959/golang-server-resources/resource"
)

var loadedAssetsPath = "engine/assets/"

func loadResource(filePath string) []byte {
	data, err := ioutil.ReadFile(filePath)
	if err != nil {
		fmt.Errorf("Error opening resource %s", filePath)
		return []byte("")
	}

	f, err := os.OpenFile(filePath+".bytes", os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0600)
	if err != nil {
		panic(err)
	}

	defer f.Close()

	for _, b := range data {
		if _, err = f.WriteString(fmt.Sprintf("%d,", b)); err != nil {
			panic(err)
		}
	}

	return data
}

func PopulateResources(res *resource.Resource) {
	res.Add(consts.AssetsCpImgLoadGif,
		"image/gif", CpImgLoadGif, 1)
	res.Add(consts.AssetsCpCodeMirrorCss,
		"text/css", CpCodeMirrorCss, 1)
	res.Add(consts.AssetsCpCodeMirrorJs,
		"application/javascript; charset=utf-8", CpCodeMirrorJs, 1)
	res.Add(consts.AssetsCpStylesCss,
		"text/css", CpStylesCss, 1)
	res.Add(consts.AssetsCpWysiwygPellCss,
		"text/css", CpWysiwygPellCss, 1)
	res.Add(consts.AssetsCpWysiwygPellJs,
		"application/javascript; charset=utf-8", CpWysiwygPellJs, 1)
	res.Add(consts.AssetsLightGalleryCss,
		"text/css", LightGalleryCss, 1)
	res.Add(consts.AssetsLightGalleryJs,
		"application/javascript; charset=utf-8", LightGalleryJs, 1)
	res.Add(consts.AssetsSysBgPng,
		"image/png", SysBgPng, 1)
	//res.Add(consts.AssetsSysFaveIco, "image/x-icon", SysFaveIco, 1)
	res.Add(consts.AssetsSysLogoPng,
		"image/png", SysLogoPng, 1)
	res.Add(consts.AssetsSysLogoSvg,
		"image/svg+xml", SysLogoSvg, 1)
	res.Add(consts.AssetsSysStylesCss,
		"text/css", SysStylesCss, 1)
	res.Add(consts.AssetsCpScriptsJs,
		"application/javascript; charset=utf-8", CpScriptsJs, 1)
	res.Add(consts.AssetsSysPlaceholderPng,
		"image/png", SysPlaceholderPng, 1)
	res.Add(consts.AssetsMaterialDashboardCss,
		"text/css", css.MaterialDashboardCss, 1)
	res.Add(consts.AssetsSysFaveIco,
		"image/x-icon", img.SysFaveIco, 1)
	res.Add(consts.AssetsMaterilaDemoCss,
		"text/css", demo.MaterilaDemoCss, 1)
	res.Add(consts.AssetsBootstrapMaterialDesign,
		"application/javascript; charset=utf-8", core.BoostrapMaterialDesign, 1)
	res.Add(consts.AssetsMaterilaDashboardMinJs,
		"application/javascript; charset=utf-8", js.MaterilaDashboardMinJs, 1)
	res.Add(consts.AssetsPerfectScroolbarJs,
		"application/javascript; charset=utf-8", plugins.PerfectScroolbarJs, 1)
	res.Add(consts.AssetsMomentJs,
		"application/javascript; charset=utf-8", plugins.MomentJs, 1)
	res.Add(consts.AssetsSweetalert2Js,
		"application/javascript; charset=utf-8", plugins.Sweetalert2Js, 1)
	res.Add(consts.AssetsValidateJs,
		"application/javascript; charset=utf-8", plugins.ValidateJs, 1)
	res.Add(consts.AssetsBootstrapWizardJs,
		"application/javascript; charset=utf-8", plugins.BootstrapWizardJs, 1)
	res.Add(consts.AssetsBootstrapSelectPickerJs,
		"application/javascript; charset=utf-8", plugins.BootstrapSelectPickerJs, 1)
	res.Add(consts.AssetsBootstrapDatetimePickerJs,
		"application/javascript; charset=utf-8", plugins.BootstrapDatetimePickerJs, 1)
	//res.Add(consts.AssetsJqueryDataTablesJs, "application/javascript; charset=utf-8", plugins.JqueryDataTablesJs, 1)
	res.Add(consts.AssetsBootstrapTagsInputJs,
		"application/javascript; charset=utf-8", plugins.BootstrapTagsInputJs, 1)
	res.Add(consts.AssetsJasnyBootstrapJs,
		"application/javascript; charset=utf-8", plugins.JasnyBootstrapJs, 1)
	res.Add(consts.AssetsFullcalendarJs,
		"application/javascript; charset=utf-8", plugins.FullcalendarJs, 1)
	res.Add(consts.AssetsJqueryJvectorMapJs,
		"application/javascript; charset=utf-8", plugins.JqueryJvectorMapJs, 1)
	res.Add(consts.AssetsNouisliderJs,
		"application/javascript; charset=utf-8", plugins.NouisliderJs, 1)
	res.Add(consts.AssetsArriveJs,
		"application/javascript; charset=utf-8", plugins.ArriveJs, 1)
	res.Add(consts.AssetsChartistJs,
		"application/javascript; charset=utf-8", plugins.ChartistJs, 1)
	res.Add(consts.AssetsChartMinCss,
		"text/css", css.ChartMinCss, 1)
	res.Add(consts.AssetsChartMinJs,
		"application/javascript; charset=utf-8", plugins.ChartMinJs, 1)
	res.Add(consts.AssetsUtilsJs,
		"application/javascript; charset=utf-8", plugins.UtilsJs, 1)
	res.Add(consts.AssetsBootstrapNotifyJs,
		"application/javascript; charset=utf-8", plugins.BootstrapNotifyJs, 1)
	res.Add(consts.AssetsDemoJs,
		"application/javascript; charset=utf-8", demo.DemoJs, 1)
	res.Add(consts.AssetsBootstrapMinCss,
		"text/css", css.BootstrapMinCss, 1)
	res.Add(consts.AssetsImageSidebarJpeg1,
		"image/jpeg", img.ImageSidebarJpeg1, 1)
	res.Add(consts.AssetsSuiteJs,
		"application/javascript; charset=utf-8", codebase.SuiteJs, 1)
	res.Add(consts.AssetsSuiteCss,
		"text/css", codebase.SuiteCss, 1)
	res.Add(consts.AssetsGridCss,
		"text/css", codebase.DatasetJs, 1)
	res.Add(consts.AssetsDatasetJs,
		"application/javascript; charset=utf-8", codebase.DatasetJs, 1)
	//res.Add(consts.AssetsImageSidebarJpeg2,
	//	"image/jpeg", loadResource(loadedAssetsPath+"img/sidebar-2.jpg"), 1)
	//res.Add(consts.AssetsImageSidebarJpeg3,
	//	"image/jpeg", loadResource(loadedAssetsPath+"img/sidebar-3.jpg"), 1)
	//res.Add(consts.AssetsImageSidebarJpeg4,
	//	"image/jpeg", loadResource(loadedAssetsPath+"img/sidebar-4.jpg"), 1)
}
