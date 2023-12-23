document.addEventListener('DOMContentLoaded', function() {
    // TV Toggle
    document.getElementById('tv-toggle').addEventListener('click', function() {
        // Implement TV toggle logic
        console.log("TV toggled");
    });

    // Lamp 2 Toggle
    document.getElementById('lamp2-toggle').addEventListener('click', function() {
        // Implement Lamp 2 toggle logic
        console.log("Lamp 2 toggled");
    });

    // Lamp 3 Toggle
    document.getElementById('lamp3-toggle').addEventListener('click', function() {
        // Implement Lamp 3 toggle logic
        console.log("Lamp 3 toggled");
    });
    
    document.getElementById('menu-button').addEventListener('click', function() {
        var menu = document.getElementById('slide-menu');
        var menuButton = document.getElementById('menu-button');
        if (menu.style.right === '0px') {
            menu.style.right = '-250px'; // Hide menu
            menuButton.style.right = '1%'; // Reset button position
        } else {
            menu.style.right = '0px'; // Show menu
            menuButton.style.right = 'calc(1% + var(--menu-width))'; // Move button with menu
        }
    });

    document.addEventListener('DOMContentLoaded', function() {
        var menuButton = document.getElementById('menu-button');
        var closeButton = document.getElementById('close-menu');
        var menu = document.getElementById('slide-menu');
    
        menuButton.addEventListener('click', function() {
            menu.style.right = '0px'; // Show menu
            menuButton.style.display = 'none'; // Hide menu button
        });
    });

    document.getElementById('menu-button').addEventListener('click', function() {
        var menu = document.getElementById('slide-menu');
        menu.classList.toggle('menuActive');
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
function updateSliderValue(change) {
    var slider = $(".roundslider");
    var currentValue = slider.roundSlider("getValue");
    var newValue = currentValue + change;
    slider.roundSlider("setValue", newValue);
}

$(document).ready(function() {
    // Function to update the slider value
    function updateSliderValue(sliderId, change) {
        var currentValue = $(sliderId).roundSlider("getValue");
        $(sliderId).roundSlider("setValue", currentValue + change);
    }

    // Event listener for increasing the lamp slider value
    $("#lamp-plus-btn").click(function() {
        updateSliderValue("#lamp-slider", 1);
    });

    // Event listener for decreasing the lamp slider value
    $("#lamp-minus-btn").click(function() {
        updateSliderValue("#lamp-slider", -1);
    });

    $("#heater-plus-btn").click(function() {
        updateSliderValue("#heater-slider", 1);
    });
    $("#heater-minus-btn").click(function() {
        updateSliderValue("#heater-slider", -1);
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

