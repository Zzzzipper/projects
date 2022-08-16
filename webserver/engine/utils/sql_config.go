package utils

import (
	"encoding/json"
	"os"
)

type ConfigSql struct {
	Host     string `json:"Host"`
	Port     string `json:"Port"`
	Name     string `json:"Name"`
	User     string `json:"User"`
	Password string `json:"Password"`
	DbFile   string `json:"DbFile"`
}

func IsConfigExists(file string) bool {
	return IsFileExists(file) && !IsDir(file)
}

func SqlConfigRead(file string) (*ConfigSql, error) {
	f, err := os.Open(file)
	if err == nil {
		defer f.Close()
		dec := json.NewDecoder(f)
		conf := ConfigSql{}
		err = dec.Decode(&conf)
		if err == nil {
			return &conf, err
		}
	}
	return nil, err
}

func SqlConfigWrite(file string, host string, port string, name string, user string, password string) error {
	r, err := json.Marshal(&ConfigSql{
		Host:     host,
		Port:     port,
		Name:     name,
		User:     user,
		Password: password,
	})
	if err == nil {
		f, err := os.Create(file)
		if err == nil {
			defer f.Close()
			_, err = f.WriteString(string(r))
			return err
		}
	}
	return err
}
