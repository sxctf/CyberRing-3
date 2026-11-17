package ideas

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"strconv"
	"text/template"
	"time"

	"golang.org/x/exp/slices"
	"gopkg.in/yaml.v2"
	"idea-exchange/internal/common"
	"idea-exchange/internal/infra"
)

const templateFile = "idea.tmpl"

var (
	statuses = []string{"private", "public"}

	ideaTemplate     *template.Template
	ideaTemplateErr  error
	ideaTemplateOnce = loadTemplate
)

func loadTemplate() {
	ideaTemplate, ideaTemplateErr = template.ParseFiles(templateFile)
}

func init() {
	ideaTemplateOnce()
}

type UseCase struct {
	repo *infra.Repo
}

func New(r *infra.Repo) *UseCase {
	return &UseCase{repo: r}
}

func (u *UseCase) GetIdea(ctx context.Context, requester, username, title string) (map[string]string, error) {
	ideas, err := u.repo.GetUserIdeas(ctx, username)
	if err != nil {
		return nil, fmt.Errorf("error while fetching ideas: %w", err)
	}

	friends, err := u.repo.GetFriendsList(ctx, requester)
	if err != nil {
		return nil, fmt.Errorf("error while fetching friends list: %w", err)
	}

	for _, idea := range ideas {
		t, titleOK := idea["title"]
		s, statusOK := idea["status"]
		if titleOK && statusOK && t == title {
			if s == "public" || s == "private" && slices.Contains(friends, username) || requester == username {
				return idea, nil
			}
		}
	}

	return nil, common.ErrNotExists
}

func (u *UseCase) CreateIdea(ctx context.Context, username, status, title, category, text string) error {
	exIdea, err := u.GetIdea(ctx, username, username, title)
	if exIdea != nil {
		return common.ErrAlreadyExists
	}
	if err != nil && !errors.Is(err, common.ErrNotExists) {
		return fmt.Errorf("error while fetching ideas: %w", err)
	}

	if !slices.Contains(statuses, status) {
		return common.ErrInvalidStatus
	}

	idea, err := fillIdeaTemplate(map[string]string{
		"Status":    status,
		"Title":     title,
		"Category":  category,
		"Text":      text,
		"CreatedAt": strconv.FormatInt(time.Now().UnixMilli(), 10),
	})
	if err != nil {
		return fmt.Errorf("error while filling template: %w", err)
	}

	if err = u.repo.AddIdea(ctx, username, idea); err != nil {
		return fmt.Errorf("error while adding idea: %w", err)
	}

	return nil
}

func (u *UseCase) GetUserIdeasList(ctx context.Context, username string) ([]map[string]string, error) {
	ideas, err := u.repo.GetUserIdeas(ctx, username)
	if err != nil {
		return nil, fmt.Errorf("error while fetching user ideas: %w", err)
	}

	return ideas, nil
}

func (u *UseCase) GetCategoryIdeas(ctx context.Context, username, category string) ([]map[string]string, error) {
	users, err := u.repo.GetUsersList(ctx)
	if err != nil {
		return nil, fmt.Errorf("error while fetching users list: %w", err)
	}

	friends, err := u.repo.GetFriendsList(ctx, username)
	if err != nil {
		return nil, fmt.Errorf("error while fetching friends: %w", err)
	}

	var res []map[string]string

	for _, user := range users {
		ideas, err := u.repo.GetUserIdeas(ctx, user)
		if err != nil {
			if errors.Is(err, common.ErrNotExists) {
				continue
			}
			return nil, fmt.Errorf("error while fetching user ideas: %w", err)
		}

		for _, idea := range ideas {
			s, statusOK := idea["status"]
			c, categoryOK := idea["category"]
			_, textOK := idea["text"]
			if statusOK && categoryOK && textOK && c == category {
				if s == "public" || s == "private" && slices.Contains(friends, user) || user == username {
					res = append(res, idea)
				}
			}
		}
	}

	return res, nil
}

func (u *UseCase) GetRating(ctx context.Context, sortByRecent bool, page int64) ([]map[string]string, error) {
	rating, err := u.repo.GetRating(ctx, sortByRecent, page)
	if err != nil {
		return nil, fmt.Errorf("error while fetching rating: %w", err)
	}

	return rating, nil
}

func fillIdeaTemplate(values map[string]string) (map[string]map[string]string, error) {
	if ideaTemplate == nil {

		loadTemplate()
	}
	if ideaTemplateErr != nil || ideaTemplate == nil {
		return nil, fmt.Errorf("error while parsing template: %w", ideaTemplateErr)
	}

	buf := new(bytes.Buffer)

	if err := ideaTemplate.Execute(buf, values); err != nil {
		return nil, fmt.Errorf("error while executing template: %w", err)
	}

	var v map[string]map[string]string

	if err := yaml.Unmarshal(buf.Bytes(), &v); err != nil {
		return nil, fmt.Errorf("error while unmarshalling: %w", err)
	}

	return v, nil
}

func (u *UseCase) SearchIdeas(ctx context.Context, q string) ([]map[string]string, error) {
	res, err := u.repo.SearchPublicIdeas(ctx, q)
	if err != nil {
		return nil, fmt.Errorf("error while searching ideas: %w", err)
	}
	return res, nil
}

func (u *UseCase) GetIdeaByIndex(ctx context.Context, username string, idx int) (map[string]string, error) {
	ideas, err := u.repo.GetUserIdeas(ctx, username)
	if err != nil {
		return nil, fmt.Errorf("error while fetching ideas: %w", err)
	}
	if idx < 0 || idx >= len(ideas) {
		return nil, common.ErrNotExists
	}
	return ideas[idx], nil
}

func (u *UseCase) AdminStats(ctx context.Context) (map[string]int, error) {
	stats, err := u.repo.AdminStats(ctx)
	if err != nil {
		return nil, fmt.Errorf("error while collecting stats: %w", err)
	}
	return stats, nil
}
