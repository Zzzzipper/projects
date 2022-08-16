package fetdata

import (
	"encoding/json"
	"fmt"
	"html/template"
	"server/engine/gates"
)

func EventName(d *Dashboard) string {
	return d.object.A_name
}
func EventContent(d *Dashboard) template.HTML {
	//
	// TODO: исправить поиск шлюза в зависимости о запроса
	//
	conn := d.wrap.GatePool.Get("scdgate", d.wrap.S.GetInt("UserId", -1))
	jsonString := ""
	if conn != nil {
		jsonString, _ = conn.(gates.IGate).Infrastructure()
	}

	//fmt.Println(jsonString)
	var content []interface{}
	err := json.Unmarshal([]byte(jsonString), &content)
	if err != nil {
		fmt.Errorf(err.Error())
		return template.HTML("404: " + err.Error())
	}
	//fmt.Printf("Content: %s \n", content)

	head := `<thead><tr role="row">`
	foot := `<tfoot><tr>`
	body := `<tbody>`
	order := "odd"
	tdStyle := `font-size:0.72em!important; line-height: 1.3; overflow: hidden;`
	var headCompleted = false
	for _, r := range content {
		tr := `<tr role="row" class="` + order + `">`
		line := r.(map[string]interface{})
		for n, c := range eventColumns {
			if c.show == true {
				if headCompleted == false {
					// style="width: 292px;"
					head += `<th class="sorting" tabindex="` + string(n) + `" aria-controls="datatables" rowspan="1" colspan="1" aria-label="` + c.name + `: activate to sort column ascending">` + c.name + `</th>`
					foot += `<th rowspan="1" colspan="1">` + c.name + `</th>`
				}
				tr += `<td class="" tabindex="` + string(n) + `" style="` + tdStyle + `">` + line[c.alias].(string) + `</td>`
			}
		}
		head += `</tr></thead>`
		foot += `</tr></foot>`
		//} else {
		//	for _, c := range headerArray {
		//		tr += `<td class="" tabindex="0" style="`+ tdStyle +`">` + line[c].(string) + `</td>`
		//	}
		//}
		tr += `</tr>`
		body += tr
		if order == "odd" {
			order = "even"
		} else {
			order = "odd"
		}
		if headCompleted == false {
			headCompleted = true
		}
	}
	body += `</tbody>`
	html := `
	<div class="card">
		<div class="card-header card-header-primary card-header-icon">
			<div class="card-icon">
				<i class="material-icons notranslate">assignment</i>
			</div>
			<h4 class="card-title">` + EventMetaTitle(d) + `</h4>
		</div>
		<div class="card-body">
			<div class="toolbar">
				<!-- Here you can write extra buttons/actions for the toolbar -->
			</div>
			<div class="material-datatables">
			<div id="datatables_wrapper" class="dataTables_wrapper dt-bootstrap4">
				<div class="row">
					<!--<div class="col-sm-12 col-md-6">
						<div class="dataTables_length" id="datatables_length">
							<label>
								Количество
								<select name="datatables_length" aria-controls="datatables" class="custom-select custom-select-sm form-control form-control-sm">
									<option value="10">10</option>
									<option value="25">25</option>
									<option value="50">50</option>
									<option value="-1">All</option>
								</select>
								строк
							</label>
						</div>
					</div>-->
					<!--<div class="col-sm-12 col-md-6">
						<div id="datatables_filter" class="dataTables_filter">
							<label>
								<span class="bmd-form-group bmd-form-group-sm">
									<input type="search" class="form-control form-control-sm" placeholder="Поиск записей" aria-controls="datatables">
								</span>
							</label>
						</div>
					</div>-->
				</div>
				<div class="row">
					<div class="col-sm-12">
						<div class="table-responsive">
							<table id="datatables" class="table table-striped table-no-bordered table-hover dataTable dtr-inline" cellspacing="0" width="100%" style="width: 100%;" role="grid" aria-describedby="datatables_info">` +
		head + foot + body +
		`</table>
						</div>
					</div>
				</div>
				<div class="row">
					<!--<div class="col-sm-12 col-md-5">
						<div class="dataTables_info" id="datatables_info" role="status" aria-live="polite">
							Showing 31 to 40 of 40 entries
						</div>
					</div>-->
					<!--<div class="col-sm-12 col-md-7">
						<div class="dataTables_paginate paging_full_numbers" id="datatables_paginate">
							<ul class="pagination">
								<li class="paginate_button page-item first" id="datatables_first">
									<a href="#" aria-controls="datatables" data-dt-idx="0" tabindex="0" class="page-link">First</a>
								</li>
								<li class="paginate_button page-item previous" id="datatables_previous">
									<a href="#" aria-controls="datatables" data-dt-idx="1" tabindex="0" class="page-link">Prev</a>
								</li>
								<li class="paginate_button page-item ">
									<a href="#" aria-controls="datatables" data-dt-idx="2" tabindex="0" class="page-link">1</a>
								</li>
								<li class="paginate_button page-item ">
									<a href="#" aria-controls="datatables" data-dt-idx="3" tabindex="0" class="page-link">2</a>
								</li>
								<li class="paginate_button page-item ">
									<a href="#" aria-controls="datatables" data-dt-idx="4" tabindex="0" class="page-link">3</a>
								</li>
								<li class="paginate_button page-item active">
									<a href="#" aria-controls="datatables" data-dt-idx="5" tabindex="0" class="page-link">4</a>
								</li>
								<li class="paginate_button page-item next disabled" id="datatables_next">
									<a href="#" aria-controls="datatables" data-dt-idx="6" tabindex="0" class="page-link">Next</a>
								</li>
								<li class="paginate_button page-item last disabled" id="datatables_last">
									<a href="#" aria-controls="datatables" data-dt-idx="7" tabindex="0" class="page-link">Last</a>
								</li>
							</ul>
						</div>
					</div>-->
				</div>
			</div>
		</div>
	</div>
	<!-- end content-->
	</div>`

	return template.HTML(html)
}

func EventMetaTitle(d *Dashboard) string {
	return d.object.A_meta_title
}
