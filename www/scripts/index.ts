

const buttonSpin = [
    { transform: "rotate(0) scale(1)"},
    { transform: "rotate(360deg) scale(0)"},
];

const buttonTiming = {
    duration: 2000,
    iterations: 1,
}

const handleClick = (id: string) => {
    document.getElementById(id)?.animate(buttonSpin, buttonTiming);
}