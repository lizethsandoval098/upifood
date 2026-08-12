#ifndef PRODUCTO_H 
#define PRODUCTO_H

#include <string>
#include <iostream>

class BaseDatos;

using namespace std;

class Producto{
	private:
		string nombreProducto;
		string idProducto;
		int cantidad;
		float precio;

		BaseDatos& bd;

	public:
		Producto();
		Producto(string nombreProducto, string idProducto, int cantidad, float precio);

		~Producto();

		string getNombreProducto() const;
		string getIdProducto() const;
		int getCantidad() const;
		float getPrecio() const;

		void setNombreProducto();
		void setIdProducto();
		void setCantidad();
		void setPrecio();
};

#endif
		
