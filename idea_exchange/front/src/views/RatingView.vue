<template>
    <div class="container">
        <div class="controls">
            <va-button-toggle
                v-model="sort"
                :options="sortOptions"
                @update:modelValue="reload"
            />
        </div>

        <va-progress-circle v-if="loading" indeterminate color="primary" />

        <va-list style="max-width: 700px; width: 100%;">
            <va-list-label>Top authors of the Idea Exchange</va-list-label>
            <va-list-item v-for="(row, i) in rows" :key="i" class="row">
                <va-card style="width: 100%;">
                    <va-card-content>
                        <div class="name">#{{ page * pageSize + i + 1 }} · {{ row.username }}</div>
                        <div class="meta">Ideas: {{ row.ideas_count }}</div>
                        <div class="meta" v-if="row.last_title">Latest idea: {{ row.last_title }}
                            <span v-if="row.last_category">({{ row.last_category }})</span>
                        </div>
                        <div class="meta" v-if="row.last_published_at">
                            Published: {{ formatDate(row.last_published_at) }}
                        </div>
                    </va-card-content>
                </va-card>
            </va-list-item>
        </va-list>

        <p v-if="!loading && rows.length === 0">Nothing here yet</p>

        <div class="pager">
            <va-button :disabled="page === 0 || loading" @click="prev">Prev</va-button>
            <span>Page {{ page + 1 }}</span>
            <va-button :disabled="rows.length < pageSize || loading" @click="next">Next</va-button>
        </div>
    </div>
</template>

<script>
import {getRating} from "@/api/ideas";

export default {
    name: "RatingView",
    props: ['token'],
    data() {
        return {
            loading: false,
            sort: "count",
            page: 0,
            pageSize: 20,
            rows: [],
            sortOptions: [
                {label: "By number of ideas", value: "count"},
                {label: "By latest publish date", value: "recent"},
            ],
        }
    },
    methods: {
        formatDate(ms) {
            const n = Number(ms);
            if (!n) return "";
            return new Date(n).toLocaleString();
        },
        load() {
            this.loading = true;
            getRating(this.token, this.sort, this.page).then((rows) => {
                this.rows = rows || [];
                this.loading = false;
            }).catch((error) => {
                this.loading = false;
                this.$vaToast.init({message: String(error), position: 'bottom-right'});
            });
        },
        reload() {
            this.page = 0;
            this.load();
        },
        next() {
            this.page += 1;
            this.load();
        },
        prev() {
            if (this.page > 0) {
                this.page -= 1;
                this.load();
            }
        },
    },
    mounted() {
        this.load();
    }
}
</script>

<style scoped>
.container {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 20px;
    gap: 16px;
}
.row {
    margin-top: 12px;
}
.name {
    font-weight: bold;
    color: rgb(180, 86, 192);
}
.meta {
    font-size: 14px;
    color: #555;
    margin-top: 4px;
}
.pager {
    display: flex;
    gap: 16px;
    align-items: center;
    margin-top: 10px;
}
</style>
