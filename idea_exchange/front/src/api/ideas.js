import {validateResponse} from "@/api/auth";

export async function saveNewIdea(token, title, category, status, text){
    const formData = new FormData();
    formData.append('title', title);
    formData.append('category', category);
    formData.append('status', status);
    formData.append('text', text);
    const response = await fetch('/api/ideas/', {
        method: 'POST',
        body: formData,
        headers: {
            'Authorization' : `Bearer ${token}`
        }
    });
    await validateResponse(response);
    return await response.json();
}

export async function getMyIdeas(token){
    const response = await fetch('/api/ideas/list', {
        method: 'GET',
        headers: {
            'Authorization' : `Bearer ${token}`
        }
    });
    await validateResponse(response);
    return (await response.json()).data;
}

export async function searchIdeas(token, category){
    const response = await fetch(`/api/ideas/list/${encodeURIComponent(category)}`, {
        method: 'GET',
        headers: {
            'Authorization' : `Bearer ${token}`
        }
    });
    await validateResponse(response);
    return (await response.json()).data;
}

export async function getRating(token, sort = "count", page = 0){
    const response = await fetch(`/api/ideas/rating?sort=${encodeURIComponent(sort)}&page=${page}`, {
        method: 'GET',
        headers: {
            'Authorization' : `Bearer ${token}`
        }
    });
    await validateResponse(response);
    return (await response.json()).data;
}
