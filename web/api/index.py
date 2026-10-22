"""Entrada ASGI da função Python no Vercel."""
from src.api.app import app

__all__ = ["app"]
