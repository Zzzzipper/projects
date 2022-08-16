package fetdata

import (
	"html/template"
)

func RetroName(d *Dashboard) string {
	return "Ретроспектива"
}

func RetroContent(d *Dashboard) template.HTML {
	return template.HTML(`<script src="/modules/retro/vue-settings.js"></script>
		<script type="module" src="/modules/retro/index.js" async defer></script>
		<div class="example cursor" id="app"></div>`)
}

func RetroMetaTitle(d *Dashboard) string {
	return d.object.A_meta_title
}
