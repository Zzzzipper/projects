import React from 'react';

const apiUrl = 'http://localhost:8080';

function postJson(method, data) {
    return fetch(`${apiUrl}/${method}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: JSON.stringify(data)
    }).then(result => result.json())
        .catch(error => {
            alert(error);
        });
};

export function checkStatus() {
    return postJson('status', null);
};

export function login(login, password) {
    return postJson('login', { login: login, password: password });
};

export function logout() {
    return postJson('logout', null);
}


export function createModule(libraryName, moduleName) {
    const params = {
        command: 'create_module',
        message: JSON.stringify({ library: libraryName, module: moduleName })
    };

    return postJson('request', params);
}

export function pingModule(moduleId) {
    const params = {
        command: 'ping',
        mod_id: moduleId
    }
    return postJson('request', params);
}

export function getRequest(moduleId, requestName, requestParams) {
    const params = {
        command: requestName,
        message: JSON.stringify(requestParams),
        mod_id: moduleId
    }
    return postJson('request', params);
}

