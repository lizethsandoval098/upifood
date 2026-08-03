#ifndef PRODUCTO_H 
#define PRODUCTO_H

#include <string>
#include <iostream>

using namespace std;

class Producto{
	private:
		string nombreProducto;
		string idProducto;
		int cantidad;
		float precio;

	public:
		Producto();
		Producto(string nombreProducto, string idProducto, int cantidad, float precio);

		~Producto();

		string getNombreProducto();
		string getIdProducto();
		int getCantidad();
		float getPrecio();

		void setNombreProducto();
		void setIdProducto();
		void setCantidad();
		void setPrecio();
};

#endif
		
