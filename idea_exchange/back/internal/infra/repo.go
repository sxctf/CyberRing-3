package infra

import (
	"context"
	"fmt"
	"time"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"idea-exchange/internal/common"
)

const ratingPageSize = 20

type Repo struct {
	col *mongo.Collection
}

func NewRepo(col *mongo.Collection) *Repo {
	return &Repo{col: col}
}

func (r *Repo) GetUser(ctx context.Context, username string) (map[string]string, error) {
	projection := options.FindOne().SetProjection(map[string]int{"username": 1, "password": 1, "permissions": 1})
	res := r.col.FindOne(ctx, map[string]string{"username": username}, projection)
	if err := res.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return nil, common.ErrNotExists
		}
		return nil, fmt.Errorf("error while finding user: %w", err)
	}

	var u map[string]string

	if err := res.Decode(&u); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	return u, nil
}

func (r *Repo) AddUser(ctx context.Context, u map[string]string) error {
	if _, err := r.col.InsertOne(ctx, u); err != nil {
		return fmt.Errorf("error while adding user: %w", err)
	}

	return nil
}

func (r *Repo) CheckUserExists(ctx context.Context, username string) (bool, error) {
	err := r.col.FindOne(ctx, map[string]string{"username": username}).Err()
	if err != nil {
		if err == mongo.ErrNoDocuments {
			return false, nil
		}
		return false, fmt.Errorf("error while findind user: %w", err)
	}

	return true, nil
}

func (r *Repo) GetUsersList(ctx context.Context) ([]string, error) {
	projection := options.Find().SetProjection(map[string]int{"_id": 0, "username": 1})
	res, err := r.col.Find(ctx, map[string]string{}, projection)
	if err != nil {
		return nil, fmt.Errorf("error while fetching users list: %w", err)
	}

	var a []map[string]string

	if err = res.All(ctx, &a); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	var users []string

	for _, u := range a {
		users = append(users, u["username"])
	}

	return users, nil
}

func (r *Repo) GetFriendsList(ctx context.Context, username string) ([]string, error) {
	projection := options.FindOne().SetProjection(map[string]int{"_id": 0, "friends": 1})
	res := r.col.FindOne(ctx, map[string]string{"username": username}, projection)
	if err := res.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return nil, common.ErrNotExists
		}
		return nil, fmt.Errorf("error while fetching user friends: %w", err)
	}

	var a map[string][]map[string]string

	if err := res.Decode(&a); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	friends := make([]string, 0)

	friendsList, ok := a["friends"]
	if ok {
		for _, f := range friendsList {
			for k := range f {
				friends = append(friends, k)
				break
			}
		}
	}

	return friends, nil
}

func (r *Repo) GetUserIdeas(ctx context.Context, username string) ([]map[string]string, error) {
	projection := options.FindOne().SetProjection(map[string]int{"_id": 0, "ideas": 1})
	res := r.col.FindOne(ctx, map[string]string{"username": username}, projection)
	if err := res.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return nil, common.ErrNotExists
		}
		return nil, fmt.Errorf("error while finding idea: %w", err)
	}

	ideas := make(map[string][]map[string]string)

	if err := res.Decode(ideas); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	return ideas["ideas"], nil
}

func (r *Repo) AddIdea(ctx context.Context, username string, idea map[string]map[string]string) error {
	res, err := r.col.UpdateOne(ctx, map[string]string{"username": username}, map[string]map[string]map[string]string{"$push": idea})
	if err != nil {
		return fmt.Errorf("error while adding idea: %w", err)
	}
	if res.MatchedCount == 0 {
		return common.ErrNotExists
	}

	_, err = r.col.UpdateOne(ctx,
		bson.M{"username": username},
		bson.M{
			"$inc": bson.M{"ideas_count": 1},
			"$set": bson.M{"last_published_at": time.Now().UnixMilli()},
		},
	)
	if err != nil {
		return fmt.Errorf("error while updating rating counters: %w", err)
	}

	return nil
}

func (r *Repo) GetRating(ctx context.Context, sortByRecent bool, page int64) ([]map[string]string, error) {
	sortField := "ideas_count"
	if sortByRecent {
		sortField = "last_published_at"
	}

	if page < 0 {
		page = 0
	}

	opts := options.Find().
		SetSort(bson.D{{Key: sortField, Value: -1}}).
		SetSkip(page * ratingPageSize).
		SetLimit(ratingPageSize).
		SetProjection(bson.M{
			"_id":               0,
			"username":          1,
			"ideas_count":       1,
			"last_published_at": 1,
			"ideas":             bson.M{"$slice": -1},
		})

	filter := bson.M{sortField: bson.M{"$gt": 0}}

	cur, err := r.col.Find(ctx, filter, opts)
	if err != nil {
		return nil, fmt.Errorf("error while fetching rating: %w", err)
	}

	var rows []bson.M
	if err = cur.All(ctx, &rows); err != nil {
		return nil, fmt.Errorf("error while decoding rating: %w", err)
	}

	rating := make([]map[string]string, 0, len(rows))
	for _, row := range rows {
		entry := map[string]string{
			"username":          asString(row["username"]),
			"ideas_count":       asString(row["ideas_count"]),
			"last_published_at": asString(row["last_published_at"]),
		}

		if ideas, ok := row["ideas"].(bson.A); ok && len(ideas) > 0 {
			if last, ok := ideas[len(ideas)-1].(bson.M); ok {
				entry["last_title"] = asString(last["title"])
				entry["last_category"] = asString(last["category"])
			}
		}

		rating = append(rating, entry)
	}

	return rating, nil
}

func asString(v interface{}) string {
	switch t := v.(type) {
	case nil:
		return ""
	case string:
		return t
	case int32:
		return fmt.Sprintf("%d", t)
	case int64:
		return fmt.Sprintf("%d", t)
	case int:
		return fmt.Sprintf("%d", t)
	case float64:
		return fmt.Sprintf("%d", int64(t))
	default:
		return fmt.Sprintf("%v", t)
	}
}

func (r *Repo) AddFriendRequest(ctx context.Context, from, to string) error {
	res, err := r.col.UpdateOne(ctx, map[string]string{"username": to}, map[string]map[string]string{"$push": {"friend_requests": from}})
	if err != nil {
		return fmt.Errorf("error while adding friend reqeust: %w", err)
	}
	if res.MatchedCount == 0 {
		return common.ErrNotExists
	}

	return nil
}

func (r *Repo) AcceptFriendRequest(ctx context.Context, username, from string) error {
	res, err := r.col.UpdateOne(ctx, map[string]string{"username": username}, map[string]map[string]string{"$pull": {"friend_requests": from}})
	if err != nil {
		return fmt.Errorf("error while deleting friend request: %w", err)
	}
	if res.MatchedCount == 0 {
		return common.ErrNotExists
	}

	return nil
}

func (r *Repo) MakeFriends(ctx context.Context, user1, user2 string) error {
	res, err := r.col.UpdateOne(ctx, map[string]string{"username": user1}, map[string]map[string]map[string]string{"$push": {"friends": {user2: "yes"}}})
	if err != nil {
		return fmt.Errorf("error while adding friend to user1: %w", err)
	}
	if res.MatchedCount == 0 {
		return common.ErrNotExists
	}

	res, err = r.col.UpdateOne(ctx, map[string]string{"username": user2}, map[string]map[string]map[string]string{"$push": {"friends": {user1: "yes"}}})
	if err != nil {
		return fmt.Errorf("error while adding friend to user2: %w", err)
	}
	if res.MatchedCount == 0 {
		return common.ErrNotExists
	}

	return nil
}

func (r *Repo) GetRequestsList(ctx context.Context, username string) ([]string, error) {
	projection := options.FindOne().SetProjection(map[string]int{"_id": 0, "friend_requests": 1})
	res := r.col.FindOne(ctx, map[string]string{"username": username}, projection)
	if err := res.Err(); err != nil {
		if err == mongo.ErrNoDocuments {
			return nil, common.ErrNotExists
		}
		return nil, fmt.Errorf("error while finding friend requests: %w", err)
	}

	var a map[string][]string

	if err := res.Decode(&a); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	requests := make([]string, 0)

	requestsList, ok := a["friend_requests"]
	if ok {
		for _, re := range requestsList {
			requests = append(requests, re)
		}
	}

	return requests, nil
}

// SearchPublicIdeas matches the query against published ideas.
// FIXME: the term is placed into the filter as-is; audit for NoSQL operator
// injection ($ne/$gt/$regex/$where) before shipping to production.
func (r *Repo) SearchPublicIdeas(ctx context.Context, q string) ([]map[string]string, error) {
	filter := bson.M{
		"ideas": bson.M{"$elemMatch": bson.M{
			"status": "public",
			"$or": []bson.M{
				{"title": q},
				{"category": q},
			},
		}},
	}

	opts := options.Find().SetProjection(bson.M{"_id": 0, "ideas": 1}).SetLimit(50)
	cur, err := r.col.Find(ctx, filter, opts)
	if err != nil {
		return nil, fmt.Errorf("error while searching ideas: %w", err)
	}

	var rows []map[string][]map[string]string
	if err = cur.All(ctx, &rows); err != nil {
		return nil, fmt.Errorf("error while decoding: %w", err)
	}

	res := make([]map[string]string, 0)
	for _, row := range rows {
		for _, idea := range row["ideas"] {
			if idea["status"] == "public" && (idea["title"] == q || idea["category"] == q) {
				res = append(res, idea)
			}
		}
	}

	return res, nil
}

func (r *Repo) AdminStats(ctx context.Context) (map[string]int, error) {
	users, err := r.col.CountDocuments(ctx, bson.M{})
	if err != nil {
		return nil, fmt.Errorf("error while counting users: %w", err)
	}

	authors, err := r.col.CountDocuments(ctx, bson.M{"ideas_count": bson.M{"$gt": 0}})
	if err != nil {
		return nil, fmt.Errorf("error while counting authors: %w", err)
	}

	return map[string]int{
		"users":          int(users),
		"authors":        int(authors),
		"schema_version": 3,
	}, nil
}
