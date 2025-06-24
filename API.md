### API DOCS
## GET /api/ping
Responds with "pong!" if server is online
## GET /*
Respond with all client static files.
## POST /api/submit
#### Arguments:
```json
{
    "mood": int // int - value from 0 to 5
};
```
Submits mood and adds it to the .json file.