"""Entrada ASGI da função Python no Vercel."""
# Reexporta a mesma aplicação ASGI usada localmente para a função serverless do Vercel.
from src.api.app import app

__all__ = ["app"]
