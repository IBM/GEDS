/**
 * Copyright 2023- Technical University of Munich. All rights reserved
 * SPDX-License-Identifier: Apache-2.0
 */

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
	}
	viper.AutomaticEnv()
	viper.SetEnvPrefix("GEDS")
	if err = viper.BindEnv("PUBSUB_MDS_ENABLED"); err != nil {
		logger.ErrorLogger.Println(err)
	}
	if err = viper.BindEnv("PERSISTENT_STORAGE_MDS_ENABLED"); err != nil {
		logger.ErrorLogger.Println(err)
	}
	if pubSubEnabled := viper.GetString("PUBSUB_MDS_ENABLED"); pubSubEnabled == "true" {
		viper.Set("PUBSUB_ENABLED", true)
	} else if pubSubEnabled == "false" {
		viper.Set("PUBSUB_ENABLED", false)
	}
	if storageEnabled := viper.GetString("PERSISTENT_STORAGE_MDS_ENABLED"); storageEnabled == "true" {
		viper.Set("PERSISTENT_STORAGE_ENABLED", true)
	} else if storageEnabled == "false" {
		viper.Set("PERSISTENT_STORAGE_ENABLED", false)
	}
	err = viper.WriteConfig()
	if err != nil {
		return &Configuration{}, err
	}
	config := &Configuration{}
	err = viper.Unmarshal(config)
	if err != nil {
		return &Configuration{}, err
	}
	return config, nil
}
