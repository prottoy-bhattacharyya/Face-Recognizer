from django.apps import AppConfig
import os
import requests
from ultralytics import YOLO

class ApiConfig(AppConfig):
    name = 'api'
    verbose_name = 'Face Recognition API'
    # download yolov8n-face.pt if it doesn't exist
    def ready(self):
        model_path = os.path.join("models", 'yolov8n-face.pt')
        face_model_url = "https://github.com/YapaLab/yolo-face/releases/download/1.0.0/yolov8n-face.pt"
        if not os.path.exists(model_path):
            print("Downloading yolov8n-face.pt model...")
            response = requests.get(face_model_url, stream=True)
            with open(model_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    f.write(chunk)
                    
            print("yolov8n-face.pt model downloaded and saved.")
        else:
            print("yolov8n-face.pt model already exists.")
