from django.shortcuts import render

import os
import cv2
import shutil
from django.conf import settings

from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework import status
from ultralytics import YOLO
from deepface import DeepFace

# 1. Initialize YOLOv8 Face Model (Downloads automatically if missing)
# yolov8n-face.pt is fine-tuned specifically for faces, much better than standard yolov8n

model_path = os.path.join("models", 'yolov8n-face.pt')
yolo_model = YOLO(model_path) 

# 2. Set up the Database Directory
DB_PATH = os.path.join(settings.BASE_DIR, 'face_database')
os.makedirs(DB_PATH, exist_ok=True)

def capture_and_crop_face():
    cap = cv2.VideoCapture(settings.CAMERA_STREAM_URL)
    
    try:
        ret, frame = cap.read()
        if not ret:
            return None, "Failed to capture video frame from camera."

        # Run YOLO detection
        results = yolo_model(frame, conf=0.5, verbose=False)
        
        # Check if any faces were detected
        if len(results[0].boxes) == 0:
            return None, "No face detected in the frame."

        # Get the highest confidence face bounding box
        box = results[0].boxes[0].xyxy[0].cpu().numpy().astype(int)
        x1, y1, x2, y2 = box
        
        # Ensure coordinates are within frame bounds
        h, w = frame.shape[:2]
        x1, y1 = max(0, x1), max(0, y1)
        x2, y2 = min(w, x2), min(h, y2)

        # Crop the face from the frame
        face_crop = frame[y1:y2, x1:x2]
        return face_crop, None
        
    finally:
        cap.release()

def clear_deepface_cache():
    for file in os.listdir(DB_PATH):
        if file.endswith(".pkl"):
            os.remove(os.path.join(DB_PATH, file))

# ==========================================
# API ENDPOINTS
# ==========================================

def index(request):
    return render(request, 'index.html')

@api_view(['POST', 'DELETE'])
def manage_user_face(request, name):
    user_folder = os.path.join(DB_PATH, name)

    # POST /faces/{name} : Enroll a face
    if request.method == 'POST':
        face_crop, error = capture_and_crop_face()
        
        if error:
            return Response({"status": "failed", "description": error}, status=status.HTTP_400_BAD_REQUEST)

        # Create user directory and save image
        os.makedirs(user_folder, exist_ok=True)
        img_path = os.path.join(user_folder, f"{name}_1.jpg")
        cv2.imwrite(img_path, face_crop)
        
        clear_deepface_cache() # Force database refresh

        return Response({
            "status": "success", 
            "name": name
        }, status=status.HTTP_201_CREATED)

    # DELETE /faces/{name} : Remove a face
    elif request.method == 'DELETE':
        if os.path.exists(user_folder):
            shutil.rmtree(user_folder)
            clear_deepface_cache()
            return Response({
                "status": "success", 
                "message": "User has been successfully removed."
            }, status=status.HTTP_200_OK)
        else:
            return Response({
                "status": "failed", 
                "description": "Not Found (The user does not exist in the database)"
            }, status=status.HTTP_404_NOT_FOUND)


@api_view(['DELETE'])
def reset_database(request):
    # DELETE /faces : Delete all faces
    if os.path.exists(DB_PATH):
        shutil.rmtree(DB_PATH)
    os.makedirs(DB_PATH, exist_ok=True)
    
    return Response({
        "status": "success", 
        "message": "All faces have been successfully removed."
    }, status=status.HTTP_200_OK)


@api_view(['GET'])
def verify_face(request):
    # GET /verify : Verify a face using YOLO + DeepFace
    try:
        threshold = float(request.GET.get('threshold', 0.5))
    except ValueError:
        return Response({"status": "failed", "description": "Invalid threshold parameter."}, status=status.HTTP_400_BAD_REQUEST)

    face_crop, error = capture_and_crop_face()
    if error:
        return Response({"status": "failed", "description": error}, status=status.HTTP_400_BAD_REQUEST)

    # If the database is empty, DeepFace will crash. Catch this early.
    subfolders = [f for f in os.listdir(DB_PATH) if os.path.isdir(os.path.join(DB_PATH, f))]
    if not subfolders:
        return Response({"status": "failed", "description": "Database is empty. Enroll a user first."}, status=status.HTTP_400_BAD_REQUEST)

    try:
        # Run DeepFace on the YOLO-cropped image. 
        # enforce_detection=False because YOLO already did the detection.
        dfs = DeepFace.find(
            img_path=face_crop, 
            db_path=DB_PATH, 
            enforce_detection=False,
            silent=True
        )

        if len(dfs) > 0 and not dfs[0].empty:
            best_match = dfs[0].iloc[0]
            distance = best_match['distance']
            
            # The identity string looks like "face_database/Prottoy/Prottoy_1.jpg"
            # We split it to get the folder name (the user's name)
            matched_name = best_match['identity'].split(os.path.sep)[-2]

            if distance <= threshold:
                return Response({
                    "status": "success",
                    "name": matched_name,
                    "confidence_score": round(1.0 - distance, 2) # Crude conversion of distance to confidence
                }, status=status.HTTP_200_OK)
            else:
                return Response({
                    "status": "failed",
                    "description": "Face not recognized or confidence below threshold.",
                    "confidence_score": round(1.0 - distance, 2)
                }, status=status.HTTP_401_UNAUTHORIZED)
        else:
            return Response({
                "status": "failed",
                "description": "No match found in the database."
            }, status=status.HTTP_401_UNAUTHORIZED)

    except Exception as e:
        return Response({"status": "failed", "description": str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)