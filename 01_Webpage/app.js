// Define the WebSocket connection
//TODO: Replace 192.168.1.100:8000 with the actual IP address and port number
var webSocket = new WebSocket("ws://192.168.1.139:8000");

// Connection opened
webSocket.onopen = function(event) {
    console.log("Connection opened", event);
};

// Connection error handling
webSocket.onerror = function(error) {
    console.log("WebSocket Error", error);
};

// Handling messages from the server
webSocket.onmessage = function(message) {
var received_msg = message.data;
    console.log("Message received: ", received_msg);
};

// Function to send a JSON command to the server
function sendCommand(commandObject) {
    var commandJSON = JSON.stringify(commandObject);
    webSocket.send(commandJSON);
    console.log("Command sent: ", commandJSON);
}

// 
function readAllData() {
    sendCommand({ action: "read", utility: "all" });
}

function readUtilityData(utility) {
    sendCommand({ action: "read", utility: utility });
}

function setUtilityStatus(utility, status) {
    sendCommand({ action: "set", utility: utility, status: status });
}

function toggleTV() {
    sendCommand({ action: "toggle", utility: "tv" });
    console.log("TV toggled");
}

function toggleAlarm() {
    sendCommand({ action: "toggle", utility: "alarm" });
    console.log("Alarm toggled");
}

function toggleLamp2() {
    sendCommand({ action: "toggle", utility: "lamp2" });
    console.log("Lamp 2 toggled");
}

function toggleLamp3() {
    sendCommand({ action: "toggle", utility: "lamp3" });
    console.log("Lamp 3 toggled");
}

function setSliderValue(utility, value) {
    sendCommand({ action: "set", utility: utility, value: value });
    console.log(`${utility} set to`, value);
}

// ----------------------------------------------------------------------------------------------------------

document.addEventListener('DOMContentLoaded', function() {
    // TV Toggle
    document.getElementById('tv-toggle').addEventListener('click', toggleTV);

    // Heater heater-slider
    document.getElementById('heater-slider').addEventListener('input', function() {
        setSliderValue("heater", this.value);
    });

    // Alarm Toggle
    document.getElementById('alarm-toggle').addEventListener('click', toggleAlarm);

    // Lamp1 Slider
    document.getElementById('lamp-slider').addEventListener('input', function() {
        setSliderValue("lamp1", this.value);
    });

    // Lamp2 Toggle
    document.getElementById('lamp2-toggle').addEventListener('click', toggleLamp2);

    // Lamp3 Toggle
    document.getElementById('lamp3-toggle').addEventListener('click', toggleLamp3);

    // Heater and Lamp Sliders
    $("#heater-plus-btn").click(function() {
        updateAndSendSliderValue("#heater-slider", 1, "heater");
    });
    $("#heater-minus-btn").click(function() {
        updateAndSendSliderValue("#heater-slider", -1, "heater");
    });
    $("#lamp-plus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", 1, "lamp1");
    });
    $("#lamp-minus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", -1, "lamp1");
    });

    // Menu Button Click
    document.getElementById('menu-button').addEventListener('click', function() {
        var menu = document.getElementById('slide-menu');
        menu.classList.toggle('menuActive');
    });
    
    // document.getElementById('menu-button').addEventListener('click', function() {
    //     var menu = document.getElementById('slide-menu');
    //     var menuButton = document.getElementById('menu-button');
    //     if (menu.style.right === '0px') {
    //         menu.style.right = '-250px'; // Hide menu
    //         menuButton.style.right = '1%'; // Reset button position
    //     } else {
    //         menu.style.right = '0px'; // Show menu
    //         menuButton.style.right = 'calc(1% + var(--menu-width))'; // Move button with menu
    //     }
    // });

    // document.addEventListener('DOMContentLoaded', function() {
    //     var menuButton = document.getElementById('menu-button');
    //     var closeButton = document.getElementById('close-menu');
    //     var menu = document.getElementById('slide-menu');
    
    //     menuButton.addEventListener('click', function() {
    //         menu.style.right = '0px'; // Show menu
    //         menuButton.style.display = 'none'; // Hide menu button
    //     });
    // });

    // SUN Button clicked
    document.getElementById('sun-btn').addEventListener('click', function() {
        console.log("SUN button clicked");
    });

    // CLOUD Button clicked
    document.getElementById('cloud-btn').addEventListener('click', function() {
        console.log("CLOUD button clicked");
    });

    // RAIN Button clicked
    document.getElementById('rain-btn').addEventListener('click', function() {
        console.log("RAIN button clicked");
    });

    // SNOW Button clicked
    document.getElementById('snow-btn').addEventListener('click', function() {
        console.log("SNOW button clicked");
    });

    // GHOST Button clicked
    document.getElementById('ghost-btn').addEventListener('click', function() {
        console.log("GHOST button clicked");
    });

    // AUTO Button clicked
    document.getElementById('auto-btn').addEventListener('click', function() {
        console.log("AUTO button clicked");
    });

    // Weather and Time Update
    updateTime();
    updateDate();
    updateWeather();

    // Adjust Slider Size
    adjustSliderSize();
    $(window).resize(adjustSliderSize);
});

function updateTime() {
    var now = new Date();
    var formattedTime = now.getHours() + ':' + now.getMinutes().toString().padStart(2, '0');
    document.getElementById('time').textContent = formattedTime;
    setTimeout(updateTime, 1000); // Update the time every second
}

updateTime(); // Initialize the time display


function updateDate() {
    var now = new Date();
    var formattedDate = now.getDate() + '.' + now.getMonth() + '.' + now.getFullYear();
    document.getElementById('date').textContent = formattedDate;
    setTimeout(updateTime, 1000); // Update the time every second
}

updateDate(); // Initialize the time display

function updateWeather() {
    fetch('https://api.openweathermap.org/data/2.5/weather?lat=47.14&lon=7.25&appid=74678fe98ac424208ce3911f63e341ad')
        .then(response => response.json())
        .then(data => {
            var temperatureInKelvin = data.main.temp; // Get temperature in Kelvin
            var temperatureInCelsius = temperatureInKelvin - 273.15; // Convert to Celsius
            var iconCode = data.weather[0].icon; // Get the icon code from the API response
            var iconUrl = 'https://openweathermap.org/img/wn/' + iconCode + '.png'; // Construct icon URL

            // Set the weather text and icon
            document.getElementById('weather').innerHTML = 
                'Weather: ' + temperatureInCelsius.toFixed(0) + ' °C ' + 
                '<img src="' + iconUrl + '" alt="Weather Icon">';
        })
        .catch(error => console.log('Error fetching weather data:', error));
}

updateWeather(); // Initialize weather display


$(document).ready(function() {
    $('#button-container button').click(function() {
        $(this).toggleClass('button-toggled');
    });
});

// Slider V2

// Function to update the slider value
function updateAndSendSliderValue(sliderId, change, utility) {
    var currentValue = $(sliderId).roundSlider("getValue");
    var newValue = currentValue + change;
    $(sliderId).roundSlider("setValue", newValue);
    setSliderValue(utility, newValue);
}

$(document).ready(function() {
    // Event listener for increasing the heater slider value
    $("#heater-plus-btn").click(function() {
        updateAndSendSliderValue("#heater-slider", 1, "heater");
    });

    // Event listener for decreasing the heater slider value
    $("#heater-minus-btn").click(function() {
        updateAndSendSliderValue("#heater-slider", -1, "heater");
    });

    // Event listener for increasing the lamp slider value
    $("#lamp-plus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", 1, "lamp1");
    });

    // Event listener for decreasing the lamp slider value
    $("#lamp-minus-btn").click(function() {
        updateAndSendSliderValue("#lamp-slider", -1, "lamp1");
    });
});



function adjustSliderSize() {
    var viewportWidth = $(window).width();
    
    // Calculate sizes based on viewport height or width
    var radius = viewportWidth * 0.06; // Example: 6% of viewport width
    var width = viewportWidth * 0.005; // Example: 0.5% of viewport width
    var handleSize = viewportWidth * 0.015; // Example: 1.5% of viewport width

    $(".roundslider").roundSlider({
    radius: radius,
    width: width,
    handleSize: handleSize,
    sliderType: "min-range",
    value: 25,
    tooltipFormat: "tooltipVal1"
    });
}

function tooltipVal1(args) {
    return args.value + "%";
}
      
// Adjust the slider size on document ready and when the window is resized
$(document).ready(adjustSliderSize);
$(window).resize(adjustSliderSize);

