<template>
    <div class="container">
        <va-card>
            <va-card-content>
                <va-card-title>Idea from the exchange</va-card-title>
                    <div class="title">{{idea.title}}
                    </div>
                    <div class="category" v-if="idea.category">
                        Category: {{idea.category}}
                    </div>
                    <div class="text">
                        <p class="text__words">
                            {{idea.text}}
                        </p>
                    </div>
                    <p>
                        Status: {{idea.status}}
                    </p>
                </va-card-content>
        </va-card>
    </div>
</template>

<script>
export default {
    name: "IdeaCard",
    props: {
        idea :{
            type: Object,
            required: true
        }
    }
}
</script>

<style scoped>
.container{
    margin: 0 auto;
    min-width: 600px;
}
.container div {
    width: 600px;
}
.title{
    font-size: 24px;
    font-weight: bold;
    margin-bottom: 10px;
    color: rgb(180, 86, 192);
    line-height: 28px;
}
.category{
    font-size: 14px;
    color: #888;
    margin-bottom: 20px;
}
.text{
    font-size: 16px;
    margin-bottom: 20px;
}
.text__words{
    margin-top: 10px;
    white-space: pre-wrap;
}
</style>
