// Define the WebSocket connection
var webSocket = new WebSocket("ws://192.168.1.139:8000");

// WebSocket event handlers

// Call readAllData() after the WebSocket connection is established
webSocket.onopen = function(event) {
    console.log("Connection opened", event);
    readAllData(); // Request utility data once the connection is open
};

webSocket.onerror = function(error) {
    console.log("WebSocket Error", error);
};

webSocket.onmessage = function(message) {
    console.log("Message received: ", message.data);
    var data = JSON.parse(message.data);

    if(data.type === "Success") {
        console.log("Command executed successfully");
        // Optionally, handle additional data if sent by the server
    } else if(data.type === "Error") {
        console.error("Error from server:", data.message);
    } else if(data.type === "UtilityData") {
        console.log("Utility data received:", data);
        updateUIWithUtilityData(data);
    } else {
        console.log("Unknown response type or data update");
        // Handle other types of responses or data updates
    }
};


// 
document.addEventListener('DOMContentLoaded', function() {
    // Initialize Event Listeners
    attachEventListeners();

    // Weather and Time Update
    updateTime();
    updateDate();
    updateWeather();

    // Adjust Slider Size
    adjustSliderSize();
    $(window).resize(adjustSliderSize);
});

function attachEventListeners() {
    // Event listeners for utility controls
    document.getElementById('tv-toggle').addEventListener('click', toggleTV);
    document.getElementById('alarm-toggle').addEventListener('click', toggleAlarm);
    document.getElementById('lamp2-toggle').addEventListener('click', toggleLamp2);
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
        var menuButton = document.getElementById('menu-button');
        if (menu.style.right === '0px' || menu.style.right === '') {
            menu.style.right = '-250px'; // Hide menu
            menuButton.style.right = '1%'; // Reset button position
        } else {
            menu.style.right = '0px'; // Show menu
            menuButton.style.right = 'calc(1% + 250px)'; // Move button with menu
        }
    });

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
}

// Initialize the RoundSliders
$(document).ready(function() {
    // Initialize the RoundSlider for the heater
    $("#heater-slider").roundSlider({
        sliderType: "min-range",
        value: 25, // Example initial value
        change: function(event) {
            // This function is called when the slider value changes
            setSliderValue("heater", event.value);
        }
    });

    // Initialize the RoundSlider for the lamp
    $("#lamp-slider").roundSlider({
        sliderType: "min-range",
        value: 25, // Example initial value
        change: function(event) {
            // This function is called when the slider value changes
            setSliderValue("lamp1", event.value);
        }
    });
});

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

// Function to update the slider value
function updateAndSendSliderValue(sliderId, change, utility) {
    var currentValue = $(sliderId).roundSlider("getValue");
    var newValue = currentValue + change;
    $(sliderId).roundSlider("setValue", newValue);
    setSliderValue(utility, newValue);
}

// ------------------ UI Update Functions ------------------

// Function to update the UI with utility data
function updateUIWithUtilityData(data) {
    // Update the TV state
    if(data.tvState !== undefined) {
        updateTVState(data.tvState);
    }

    // Update the Alarm state
    if(data.alarmState !== undefined) {
        updateAlarmState(data.alarmState);
    }

    // Update the lamp2 state
    if(data.led2State !== undefined) {
        updateLamp2State(data.led2State);
    }

    // Update the lamp3 state
    if(data.led3State !== undefined) {
        updateLamp3State(data.led3State);
    }

    // Update the Heater slider
    if(data.heatState !== undefined) {
        if(data.heatState === 1) {
            $("#heater-slider").roundSlider("setValue", 100);
        } else {
            $("#heater-slider").roundSlider("setValue", 0);
        }
    }

    // Update the Lamp slider, wäls fählt temp -> led1
    // TODO
    if(data.temperature !== undefined) {
        $("#lamp-slider").roundSlider("setValue", data.temperature);
    }
}

// Function to update the TV state
function updateTVState(state) {
    // Logic to update TV's UI, e.g., toggling a button class
    var tvButton = document.getElementById('tv-toggle');
    if(state === 1) {
        tvButton.classList.add("button-toggled");
    } else {
        tvButton.classList.remove("button-toggled");
    }
}

// Function to update the Alarm state
function updateAlarmState(state) {
    // Logic to update Alarm's UI, e.g., toggling a button class
    var alarmButton = document.getElementById('alarm-toggle');
    if(state === 1) {
        alarmButton.classList.add("button-toggled");
    } else {
        alarmButton.classList.remove("button-toggled");
    }
}

// Function to update the lamp2 state
function updateLamp2State(state) {
    // Logic to update lamp2's UI, e.g., toggling a button class
    var lamp2Button = document.getElementById('lamp2-toggle');
    if(state === 1) {
        lamp2Button.classList.add("button-toggled");
    } else {
        lamp2Button.classList.remove("button-toggled");
    }
}

// Function to update the lamp3 state
function updateLamp3State(state) {
    // Logic to update lamp3's UI, e.g., toggling a button class
    var lamp3Button = document.getElementById('lamp3-toggle');
    if(state === 1) {
        lamp3Button.classList.add("button-toggled");
    } else {
        lamp3Button.classList.remove("button-toggled");
    }
}


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

function updateTime() {
    var now = new Date();
    var formattedTime = now.getHours() + ':' + now.getMinutes().toString().padStart(2, '0');
    document.getElementById('time').textContent = formattedTime;
    setTimeout(updateTime, 1000); // Update the time every second
}

function updateDate() {
    var now = new Date();
    var formattedDate = now.getDate() + '.' + now.getMonth() + '.' + now.getFullYear();
    document.getElementById('date').textContent = formattedDate;
    setTimeout(updateTime, 1000); // Update the time every second
}

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

$(document).ready(function() {
    $('#button-container button').click(function() {
        $(this).toggleClass('button-toggled');
    });
});

function tooltipVal1(args) {
    return args.value + "%";
}
      
// Adjust the slider size on document ready and when the window is resized
$(document).ready(adjustSliderSize);
$(window).resize(adjustSliderSize);