from django.urls import path
from . import views

urlpatterns = [
    path('', views.login, name='login'),
    path('signup/', views.signup, name='signup'),
    path('logout/', views.logout, name='logout'),

    path('index/', views.index, name='index'),
    path('controls/', views.controls, name='controls'),
    
    # Maps to POST and DELETE
    path('faces/<str:name>/', views.manage_user_face, name='manage_user_face'),
    
    # Maps to DELETE all
    path('faces/', views.reset_database, name='reset_database'),
    
    # Maps to GET verify
    path('verify/', views.verify_face, name='verify_face'),
]