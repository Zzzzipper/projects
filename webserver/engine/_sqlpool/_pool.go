package sqlpool

import (
	"errors"
	"server/engine/sqlw"
	"strings"
	"sync"
)

type SqlPool struct {
	sync.RWMutex
	connections map[string]*sqlw.DB
}

func New() *SqlPool {
	r := SqlPool{}
	r.connections = map[string]*sqlw.DB{}
	return &r
}

func (this *SqlPool) Get(key string) *sqlw.DB {
	this.Lock()
	defer this.Unlock()
	if value, ok := this.connections[key]; ok == true {
		return value
	}
	return nil
}

func (this *SqlPool) Set(key string, value *sqlw.DB) {
	this.Lock()
	defer this.Unlock()
	this.connections[key] = value
}

func (this *SqlPool) Del(key string) {
	if _, ok := this.connections[key]; ok {
		delete(this.connections, key)
	}
}

func (this *SqlPool) Close() error {
	this.Lock()
	defer this.Unlock()
	var errs []string
	for _, c := range this.connections {
		if c != nil {
			if err := c.Close(); err != nil {
				errs = append(errs, err.Error())
			}
		}
	}
	if len(errs) > 0 {
		return errors.New(strings.Join(errs, ", "))
	}
	return nil
}
