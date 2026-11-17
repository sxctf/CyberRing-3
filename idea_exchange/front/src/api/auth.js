export async function registerUser(username, password){
    const formData = new FormData();
    formData.append('username', username);
    formData.append('password', password);
    const response = await fetch('/api/register', {
        method: 'POST',
        body: formData
    });
    await validateResponse(response);
    return await response.json();
}

export async function validateResponse(response) {
    if (!response.ok) {
        // A stale/expired token (wrong signature or a user that no longer
        // exists) makes protected endpoints answer 401/403. Instead of leaving
        // the user on a broken page, drop the token and send them to login.
        // Skipped on the login/register pages so their own errors still show.
        if (response.status === 401 || response.status === 403) {
            const onAuthPage = location.hash.indexOf('/login') !== -1
                || location.hash.indexOf('/register') !== -1;
            if (!onAuthPage) {
                try { localStorage.removeItem('token'); } catch (e) { /* ignore */ }
                location.hash = '#/login';
                location.reload();
            }
        }
        if (response.json) {
            const error = await response.json();
            throw new Error(error.error);
        }
        throw new Error(response.statusText);
    }
}


export async function loginGetToken(username, password){
    const formData = new FormData();
    formData.append('username', username);
    formData.append('password', password);
    const response = await fetch('/api/login', {
        method: 'POST',
        body: formData
    });
    await validateResponse(response);
    return (await response.json()).token;
}