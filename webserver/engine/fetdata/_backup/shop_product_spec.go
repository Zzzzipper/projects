package fetdata

import (
	"server/engine/utils"
	"server/engine/wrapper"
)

type ShopProductSpec struct {
	wrap   *wrapper.Wrapper
	object *utils.Sql_shop_product_spec
}

func (this *ShopProductSpec) load() *ShopProductSpec {
	return this
}

func (this *ShopProductSpec) FilterId() int {
	if this == nil {
		return 0
	}
	return this.object.A_filter_id
}

func (this *ShopProductSpec) FilterName() string {
	if this == nil {
		return ""
	}
	return this.object.A_filter_name
}

func (this *ShopProductSpec) FilterValue() string {
	if this == nil {
		return ""
	}
	return this.object.A_filter_value
}
