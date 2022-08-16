package config

import (
	"encoding/json"
	"os"
	"sync"
)

type Gate struct {
	ServerAddress     string `json:"ServerAddress"`
	ServerPort        int    `json:"ServerPort"`
	DialServerTimeOut int    `json:"DialServerTimeOut"`
	QueryTimeOut      int    `json:"QueryTimeOut"`
}

type Config struct {
	Engine struct {
		MainModule  int `json:"MainModule"`
		Maintenance int `json:"Maintenance"`
	} `json:"Engine"`
	Blog struct {
		Pagination struct {
			Index    int `json:"Index"`
			Category int `json:"Category"`
		} `json:"Pagination"`
	}
	Api struct {
		Xml struct {
			Enabled int    `json:"Enabled"`
			Name    string `json:"Name"`
			Company string `json:"Company"`
			Url     string `json:"Url"`
		} `json:"Xml"`
	} `json:"Api"`
	Smtp struct {
		Host     string `json:"Host"`
		Port     int    `json:"Port"`
		Login    string `json:"Login"`
		Password string `json:"Password"`
	} `json:"Smtp"`
	Modules struct {
		Enabled struct {
			Blog          int `json:"Blog"`
			EmptyPassword int `json:"EmptyPassword"`
		} `json:"Enabled"`
	} `json:"Modules"`
	Grpc struct {
		Auth  string          `json:"Auth"`
		Gates map[string]Gate `json:"Gates"`
	} `json:"Grpc"`
	DbConection struct {
		CurrDriver string `json:"CurrDriver"`
	} `json:"DbConection"`
}

var config *Config

func ConfigNew() *Config {
	if config == nil {
		config = &Config{}
		config.configDefault()
	}
	return config
}

func (this *Config) configDefault() {
	this.Engine.MainModule = 1
	this.Engine.Maintenance = 0

	this.Blog.Pagination.Index = 5
	this.Blog.Pagination.Category = 5

	this.Api.Xml.Enabled = 0
	this.Api.Xml.Name = ""
	this.Api.Xml.Company = ""
	this.Api.Xml.Url = ""

	this.Smtp.Host = ""
	this.Smtp.Port = 587
	this.Smtp.Login = ""
	this.Smtp.Password = ""

	this.Modules.Enabled.Blog = 1
	this.Modules.Enabled.EmptyPassword = 0

	this.Grpc.Auth = "scdgate"
	this.Grpc.Gates = make(map[string]Gate, 1)
	this.Grpc.Gates[this.Grpc.Auth] = Gate{"localhost", 50051, 30, 30}

	this.DbConection.CurrDriver = "mysql" // sqlite

}

var mu sync.Mutex

func (this *Config) ConfigRead(file string) error {
	mu.Lock()
	defer mu.Unlock()

	f, err := os.Open(file)
	if err != nil {
		return err
	}
	defer f.Close()

	dec := json.NewDecoder(f)
	return dec.Decode(this)
}

func (this *Config) ConfigWrite(file string) error {
	mu.Lock()
	defer mu.Unlock()

	r, err := json.Marshal(this)
	if err != nil {
		return err
	}
	f, err := os.Create(file)
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = f.WriteString(string(r))
	return err
}
