package config

import (
	"github.com/IBM/gedsmds/internal/logger"
	"github.com/google/uuid"
	"github.com/spf13/viper"
)

var Config *Configuration

type Configuration struct {
	UUID                     string `mapstructure:"UUID"`
	MDSPort                  string `mapstructure:"METADATA_SERVER_PORT"`
	PrometheusPort           string `mapstructure:"PROMETHEUS_SERVER_PORT"`
	PubSubEnabled            bool   `mapstructure:"PUBSUB_ENABLED"`
	PersistentStorageEnabled bool   `mapstructure:"PERSISTENT_STORAGE_ENABLED"`
	RepopulateCacheEnabled   bool   `mapstructure:"REPOPULATE_CACHE_ENABLED"`
	PrometheusEnabled        bool   `mapstructure:"PROMETHEUS_ENABLED"`
}

func init() {
	var err error
	Config, err = LoadConfig()
	if err != nil {
		logger.FatalLogger.Fatalln(err)
	}
}

func LoadConfig() (*Configuration, error) {
	viper.AddConfigPath(".")
	viper.AddConfigPath("./configs")
	viper.SetConfigName("app")
	viper.SetConfigType("env")
	err := viper.ReadInConfig()
	if err != nil {
		return &Configuration{}, err
	}
	appUUID := viper.GetString("UUID")
	if len(appUUID) == 0 {
		viper.Set("UUID", uuid.NewString())
		err = viper.WriteConfig()
		if err != nil {
			return &Configuration{}, err
		}
	}
	config := &Configuration{}
	err = viper.Unmarshal(config)
	if err != nil {
		return &Configuration{}, err
	}
	return config, nil
}
