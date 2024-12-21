"use strict";
const buttonSpin = [
    { transform: "rotate(0) scale(1)" },
    { transform: "rotate(360deg) scale(0)" },
];
const buttonTiming = {
    duration: 2000,
    iterations: 1,
};
const handleClick = (id) => {
    var _a;
    (_a = document.getElementById(id)) === null || _a === void 0 ? void 0 : _a.animate(buttonSpin, buttonTiming);
};
