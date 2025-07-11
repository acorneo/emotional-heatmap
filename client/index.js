const countries = document.querySelectorAll('#world-map path');

countries.forEach(country => {
    country.style.fill = "#bebebe"
    country.style.stroke = "#000";
    country.style.strokeWidth = "0.3";
});

document.addEventListener('DOMContentLoaded', () => {
    countries.forEach(country => {
        country.addEventListener('mouseenter', (e) => {
            country.style.fill = '#ffcc00';
        });

        country.addEventListener('mouseleave', (e) => {
            country.style.fill = '#bebebe'; // бебебе хахахах
        });
    });
});

function showModal() {
    document.getElementById('modal-wrapper').classList.remove('hidden');
}
function hideModal() {
    document.getElementById('modal-wrapper').classList.add('hidden');
}
document.getElementById('modal-close').addEventListener('click', hideModal);

// so lets create a cookie that kinda stores if page was opened
// and if it was not (cookie unpresented) than i should show modal and create a cookie
function getCookie(name) {
    const ca = document.cookie.split(';');
    for (let i = 0; i < ca.length; i++) { // GUYS WHY YOU HAVE NO FUCKING TYPES
        let c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
        if (c.indexOf(name+"=") == 0) return c.substring((name+"=").length,c.length);
    }
    return null; // at least you have null
}
function setCookie(name, value, days) {
    let expires = "";
    if (days) {
        const date = new Date();
        date.setTime(date.getTime() + (days*24*60*60*1000));
        expires = "; expires=" + date.toUTCString();
    }
    document.cookie = name + "=" + (value || "")  + expires + "; path=/";
}
if (getCookie("seen") === null) {
    setCookie("seen", "yes it was seen indeed", 30);
    showModal();
}


document.getElementById('commit-mood-modal-close').addEventListener('click', function(e) {
    document.getElementById('commit-mood-modal-wrapper').classList.add('hidden'); // TODO: send request to backend
    showLoadingModal();
});
document.getElementById('commit-mood').addEventListener('click', function(e) {
    document.getElementById('commit-mood-modal-wrapper').classList.remove('hidden');
});

document.addEventListener('DOMContentLoaded', () => {
    const moodBtnsContainer = document.getElementById('mood-btns-cntnr');
    if (moodBtnsContainer) {
        const emojiButtons = moodBtnsContainer.querySelectorAll('img');
        emojiButtons.forEach(button => {
            button.addEventListener('click', (e) => {
                emojiButtons.forEach(btn => btn.classList.remove('selected'));
                e.target.classList.add('selected');
            });
        });
    }
});

function showLoadingModal() {
    document.getElementById('loading-modal-wrapper').classList.remove('hidden');
}
function hideLoadingModal() {
    document.getElementById('loading-modal-wrapper').classList.add('hidden');
}

function showSuccessModal() {
    document.getElementById('success-modal-wrapper').classList.remove('hidden');
}
function hideSuccessModal() {
    document.getElementById('success-modal-wrapper').classList.add('hidden');
}
document.getElementById('success-modal-close').addEventListener('click', hideSuccessModal);

showSuccessModal();