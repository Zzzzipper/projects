package gates

import (
	"errors"
	"fmt"
	"strings"
	"sync"
)

//
// Pool - массив соединений
//
type Pool struct {
	sync.RWMutex
	gates map[int32]*interface{}
}

var pool *Pool

//
// New - создание массива
//
func New() *Pool {
	if pool == nil {
		pool = &Pool{}
		pool.gates = make(map[int32]*interface{})
	}
	return pool
}

//
// Get - взять соединение по ключу
//
func (p *Pool) Get(id int32) interface{} {
	p.Lock()
	defer p.Unlock()
	if p.gates[id] != nil {
		return (*p.gates[id])
	}
	return nil
}

//
// Set - сохранить соединение в хеше
//
func (p *Pool) Set(id int32, value interface{}) {
	fmt.Println("- Добавлен в пул шлюзов: ", id, value)
	p.Lock()
	defer p.Unlock()
	p.gates[id] = &value
}

//
// ClearByUserId
//
func (p *Pool) ClearByUserId(id int32) {
	p.Lock()
	defer p.Unlock()
	p.Del(id)
}

//
// Del - удалить соединение по ключу
//
func (p *Pool) Del(id int32) {
	delete(p.gates, id)
}

//
// Close - удалить все соединения
//
func (p *Pool) Close() error {
	p.Lock()
	defer p.Unlock()
	var errs []string
	for _, gate := range p.gates {
		if gate != nil {
			if err := (*gate).(IGate).Close(); err != nil {
				errs = append(errs, err.Error())
			}
		}
	}
	if len(errs) > 0 {
		return errors.New(strings.Join(errs, ", "))
	}
	return nil
}
