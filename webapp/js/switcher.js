let consoleSwitcherView = document.querySelector("#switcher");
let logsPanelView = document.querySelector("#logs-panel");

function swapClass(class1, class2, element) {
	let enabled1 = element.classList.toggle(class1);
	let enabled2 = element.classList.toggle(class2);
	if (enabled2 && enabled1) {
		element.classList.toggle(class1);
	}
}

function switchConsole(){
	swapClass("show", "hide", logsPanelView);
}

switcher.addEventListener('click', switchConsole);