package main

import (
	"context"
	"log"
	"os"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"idea-exchange/internal/app"
	"idea-exchange/internal/infra"
	"idea-exchange/internal/server"
	"idea-exchange/internal/usecases/auth"
	"idea-exchange/internal/usecases/friends"
	"idea-exchange/internal/usecases/ideas"
)

func getenv(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

func main() {
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()

	uri := getenv("MONGO_URI", "mongodb://mongo:27017")
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(uri))
	if err != nil {
		log.Fatal(err)
	}

	secret := getenv("JWT_SECRET", "HUI")

	col := client.Database("ideas").Collection("ideas")

	ensureIndexes(ctx, col)

	r := infra.NewRepo(col)

	a := app.New(auth.New(r, secret), ideas.New(r), friends.New(r))

	s := server.NewServer(":8000", secret, a)

	if err = s.Run(); err != nil {
		log.Fatal(err)
	}
}

func ensureIndexes(ctx context.Context, col *mongo.Collection) {
	models := []mongo.IndexModel{
		{
			Keys:    bson.D{{Key: "username", Value: 1}},
			Options: options.Index().SetUnique(true).SetName("uniq_username"),
		},
		{
			Keys:    bson.D{{Key: "ideas_count", Value: -1}},
			Options: options.Index().SetName("rating_count"),
		},
		{
			Keys:    bson.D{{Key: "last_published_at", Value: -1}},
			Options: options.Index().SetName("rating_recent"),
		},
	}

	if _, err := col.Indexes().CreateMany(ctx, models); err != nil {
		log.Printf("warning: could not ensure indexes: %v", err)
	}
}
