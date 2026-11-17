<template>
    <div class="container ">

        <va-progress-circle v-if="loading" indeterminate color="primary" />
        <IdeaCard v-for="(idea, i) in ideas" :key="i" :idea="idea"/>
        <p v-if="!loading && ideas.length === 0">You have no ideas yet</p>
    </div>
</template>

<script>
import IdeaCard from "@/components/IdeaCard.vue";
import {getMyIdeas} from "@/api/ideas";

export default {
    name: "MyIdeasView",
    components: {IdeaCard},
    props: ['token'],
    data(){
        return {
            ideas: [],
            loading: true
        }
    },
    mounted() {
        this.loading = true
        getMyIdeas(this.token).then((resp) => {
            this.ideas = resp || [];
            this.loading = false
        }).catch((error) => this.$vaToast.init({ message: String(error), position: 'bottom-right' }))
    }
}
</script>

<style scoped>
.container {
    display: flex;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    height: 100%;
    padding: 20px;
}
</style>
