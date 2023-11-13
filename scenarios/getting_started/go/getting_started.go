package main

import (
	"context"
	"crypto/tls"
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"

	"mqttapplicationsamples/ConnectionSettings"

	"github.com/eclipse/paho.golang/paho"
)

func main() {
	// Load connection settings
	var cs ConnectionSettings.MqttConnectionSettings = ConnectionSettings.LoadConnectionSettings("../.env")
	fmt.Println(cs.CaFile)
	fmt.Println(cs.KeyFile)
	// Load certificates
	fmt.Println("Loading certificates")
	cert, err := tls.LoadX509KeyPair(fmt.Sprintf("../%s", cs.CertFile), fmt.Sprintf("../%s", cs.KeyFile))
	if err != nil {
		log.Fatal(err)
	}

	cfg := &tls.Config{
		Certificates: []tls.Certificate{cert},
	}

	fmt.Println("Dialing Eventgrid")
	conn, err := tls.Dial("tcp", fmt.Sprintf("%s:%d", cs.Hostname, cs.TcpPort), cfg)
	if err != nil {
		panic(err)
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	fmt.Println("Creating Paho client")
	c := paho.NewClient(paho.ClientConfig{
		Conn: conn,
		Router: paho.NewSingleHandlerRouter(func(m *paho.Publish) {
			fmt.Printf("received message on topic %s; body: %s (retain: %t)\n", m.Topic, m.Payload, m.Retain)
		}),
		OnClientError: func(err error) { fmt.Printf("server requested disconnect: %s\n", err) },
		OnServerDisconnect: func(d *paho.Disconnect) {
			if d.Properties != nil {
				fmt.Printf("server requested disconnect: %s\n", d.Properties.ReasonString)
			} else {
				fmt.Printf("server requested disconnect; reason code: %d\n", d.ReasonCode)
			}
		},
	})

	cp := &paho.Connect{
		KeepAlive:    cs.KeepAlive,
		ClientID:     cs.ClientId,
		CleanStart:   true,
		Username:     cs.Username,
		UsernameFlag: true,
		Password:     nil,
	}

	fmt.Println("Attempting to connect")
	ca, err := c.Connect(ctx, cp)
	if err != nil {
		log.Fatalln(err)
	}
	if ca.ReasonCode != 0 {
		log.Fatalf("Failed to connect to %s : %d - %s", cs.Hostname, ca.ReasonCode, ca.Properties.ReasonString)
	}

	fmt.Printf("Connection successful")
	c.Subscribe(ctx, &paho.Subscribe{
		Subscriptions: []paho.SubscribeOptions{
			{Topic: "sample/+", QoS: byte(1)},
		},
	})

	c.Publish(context.Background(), &paho.Publish{
		Topic:   "sample/topic1",
		QoS:     byte(1),
		Retain:  false,
		Payload: []byte("hello world"),
	})

	<-ctx.Done() // Wait for user to trigger exit
	fmt.Println("signal caught - exiting")
}
