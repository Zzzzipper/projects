package fetdata

import (
	"html/template"
	"server/engine/gates"
)

func InfraName(d *Dashboard) string {
	return d.object.A_name
}
func InfraContent(d *Dashboard) template.HTML {
	//
	// TODO: исправить поиск шлюза в зависимости о запроса
	//
	conn := d.wrap.GatePool.Get("scdgate", d.wrap.S.GetInt("UserId", -1))
	jsonString := ""
	if conn != nil {
		jsonString, _ = conn.(gates.IGate).Infrastructure()
	}

	prefix := `<!-- component container -->
	<!--<div class="container">-->
		<div class="row">
			 <div class="col-sm-12 col-12 pr-0">
				 <div style="background: #fff; height: 720px; overflow-y: auto; overflow-x: hidden; border: solid 1px #aca;" id="tree"></div>
			 </div>
		 </div>
	<!--</div>-->
	
	<!-- dataset -->
	<script>
	  const treedata = `

	suffix := `;
	const tree = new dhx.Tree("tree");
	tree.data.parse(treedata);
	tree.events.on("itemdblclick", function(id) {
	  tree.editItem(id);
	});
	/*
	var grid = new dhx.Grid("grid", {
					columns: [
						{ width: 200, id: "country", header: [{ text: "Country" }, { content: "selectFilter" }] },
						{ width: 160, id: "population", header: [{ text: "Population" }] },
						{ width: 160, id: "yearlyChange", header: [{ text: "Yearly Change" }] },
						{ width: 160, id: "netChange", header: [{ text: "Net Change" }] },
						{ width: 160, id: "destiny", header: [{ text: "Density (P/Km²)" }, { content: "inputFilter"}] },
						{ width: 160, id: "area", header: [{ text: "Land Area (Km²)" }] },
						{ width: 160, id: "migrants", header: [{ text: "Migrants (net)" }, { content: "comboFilter", filterConfig: { placeholder: "Type something", virtual: true } }] },
						{ width: 160, id: "fert", header: [{ text: "Fert. Rate" }] },
						{ width: 160, id: "age", header: [{ text: "Med. Age" }] },
						{ width: 160, id: "urban", header: [{ text: "Urban Pop" }] }
					],
					data: dataset
				});
	
	var gridRight = new dhx.Grid("gridRight", {
					columns: [
						{ width: 160, id: "area", header: [{ text: "Land Area (Km²)" }] },
						{ width: 160, id: "migrants", header: [{ text: "Migrants (net)" }, { content: "comboFilter", filterConfig: { placeholder: "Type something", virtual: true } }] },
					],
					data: dataset
				});*/
	
	</script>`

	/*this.object.A_content*/
	return template.HTML(prefix + jsonString + suffix)
}

func InfraMetaTitle(d *Dashboard) string {
	return d.object.A_meta_title
}
