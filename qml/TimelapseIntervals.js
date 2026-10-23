.pragma library

function presets() {
    return ["1s", "2s", "3s", "5s", "10s", "15s", "30s", "60s", "2m", "3m", "4m", "5m", "6m", "7m", "8m", "9m", "10m"]
}

function seconds(value) {
    var match = /^(\d+(?:\.\d+)?)(s|m)$/.exec(String(value))
    return match ? Number(match[1]) * (match[2] === "m" ? 60 : 1) : NaN
}

function options(current) {
    var values = presets()
    if (values.indexOf(current) === -1 && seconds(current) >= 0.1 && seconds(current) <= 86400)
        values.push(current)
    return values.concat(["Custom..."])
}

function next(current) {
    var values = presets()
    var index = values.indexOf(current)
    if (index !== -1)
        return values[(index + 1) % values.length]
    var currentSeconds = seconds(current)
    for (var i = 0; i < values.length; ++i)
        if (seconds(values[i]) > currentSeconds)
            return values[i]
    return values[0]
}

function shutterOptions(values, interval) {
    var maximum = seconds(interval)
    if (!(maximum > 0))
        return values
    return values.filter(function(value) {
        if (value === "Auto") return true
        var exposure = value.indexOf("1/") === 0 ? 1 / Number(value.slice(2)) : seconds(value)
        return exposure > 0 && Math.round(exposure * 1000000) <= Math.round(maximum * 1000000)
    })
}
