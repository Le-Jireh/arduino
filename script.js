async function updateData() {
  const url = "https://api.thingspeak.com/channels/3191471/feeds.json?api_key=QZMVKJ1O8EW00E98&results=2";
  const response = await fetch(url)
  const data = await response.json()
  const lastUser = data.feeds[data.feeds.length - 1]

  const present = document.getElementById("present")
  const access = document.getElementById("access")
  const name = document.getElementById("name")
  const position = document.getElementById("position")
  const time = document.getElementById("time")

  occupancy.textContent = lastUser.field3
  position.textContent = lastUser.field1

  if (lastUser.field1 === "Sam entered") {
    position.textContent = "Member entered";}
  else if (lastUser.field1 === "Sam left") {
    position.textContent = "Member left";}
  else {
    position.textContent = lastUser.field1;}
  time.textContent = lastUser.field6
}
setInterval(updateData, 15000)
updateData()
