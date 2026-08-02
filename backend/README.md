# Face Recognition API

A Django-based REST API for real-time face enrollment and verification using YOLOv8 (for precise face cropping) and DeepFace (for facial embeddings and matching).

## Base URL
`http://127.0.0.1:8000` (Local Development)

---

## Endpoints Overview

| Method | Path | Description |
| :--- | :--- | :--- |
| `POST` | `/faces/{name}/` | Capture and enroll a new user's face. |
| `GET` | `/verify/` | Capture a face and verify it against the database. |
| `DELETE` | `/faces/{name}/` | Delete a specific user's facial data. |
| `DELETE` | `/faces/` | Delete all users and reset the database. |

---

## API Reference

### 1. Enroll a Face
Activates the server camera, captures a frame, detects the face using YOLOv8, and saves the crop to the database under the specified user's name.

*   **URL:** `/faces/{name}/`
*   **Method:** `POST`
*   **URL Parameters:**

| Parameter | Type | Required | Description |
| :--- | :--- | :--- | :--- |
| `name` | `string` | **Yes** | The name or ID of the user being enrolled. |

*   **Success Response:**
    *   **Code:** `201 Created`
    *   **Content:**
        ```json
        {
            "status": "success",
            "name": "Prottoy"
        }
        ```

*   **Error Responses:**
    *   **Code:** `400 Bad Request` (e.g., No face detected, camera failed)
    *   **Content:**
        ```json
        {
            "status": "failed",
            "description": "No face detected in the frame."
        }
        ```

---

### 2. Verify a Face
Activates the camera, captures a face, and compares it against all enrolled users using DeepFace.

*   **URL:** `/verify/`
*   **Method:** `GET`
*   **Query Parameters:**

| Parameter | Type | Required | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| `threshold` | `float` | No | `0.5` | The strictness of the match (lower distance = higher confidence). |

*   **Success Response (Match Found):**
    *   **Code:** `200 OK`
    *   **Content:**
        ```json
        {
            "status": "success",
            "name": "Prottoy",
            "confidence_score": 0.78
        }
        ```

*   **Error Responses:**
    *   **Code:** `401 Unauthorized` (Face not recognized or below threshold)
    *   **Content:**
        ```json
        {
            "status": "failed",
            "message": "Face not recognized or confidence below threshold.",
            "confidence_score": 0.42
        }
        ```
    *   **Code:** `400 Bad Request` (Database is empty or camera failed)

---

### 3. Remove a User
Deletes a specific user's folder and facial data from the system, and clears the DeepFace cache.

*   **URL:** `/faces/{name}/`
*   **Method:** `DELETE`
*   **URL Parameters:**

| Parameter | Type | Required | Description |
| :--- | :--- | :--- | :--- |
| `name` | `string` | **Yes** | The name of the user to remove. |

*   **Success Response:**
    *   **Code:** `200 OK`
    *   **Content:**
        ```json
        {
            "status": "success",
            "message": "User has been successfully removed."
        }
        ```

*   **Error Response:**
    *   **Code:** `404 Not Found` (User does not exist)

---

### 4. Reset Database
Deletes all enrolled users, clearing the entire `face_database` directory.

*   **URL:** `/faces/`
*   **Method:** `DELETE`
*   **URL Parameters:** None

*   **Success Response:**
    *   **Code:** `200 OK`
    *   **Content:**
        ```json
        {
            "status": "success",
            "message": "All faces have been successfully removed."
        }
        ```

---

## Status Codes Summary

*   **`200 OK`** - The request was successful.
*   **`201 Created`** - The user was successfully enrolled.
*   **`400 Bad Request`** - The request was malformed, or the camera/detection failed.
*   **`401 Unauthorized`** - Verification failed (no match found).
*   **`404 Not Found`** - The requested user to delete does not exist.
*   **`500 Internal Server Error`** - A server-side error occurred during DeepFace processing.

## Contact
**Prottoy Vhattacharyya**
prottoyvhattacharyya@gmail.com