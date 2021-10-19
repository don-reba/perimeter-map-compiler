using System;
using System.Xml;
using System.Drawing;
using System.IO;
using System.Text;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for ScriptXmlWriter.
	/// </summary>
	public class ScriptXmlWriter : System.Xml.XmlTextWriter
	{
		public ScriptXmlWriter(TextWriter w)
			:base(w)
		{}
		public ScriptXmlWriter(Stream w, Encoding encoding)
			:base(w, encoding)
		{}
		public ScriptXmlWriter(string filename, Encoding encoding)
			:base(filename, encoding)
		{}

		public void WriteElement(string type, string name, string value)
		{
			WriteStartNamedElement(type, name);
			WriteString(value);
			WriteEndElement();
		}

		public void WritePoint(string name, Point point)
		{
			WriteStartNamedElement("set", name);
			WriteElement("int", "x", point.X.ToString());
			WriteElement("int", "y", point.Y.ToString());
			WriteEndElement();
		}

		public void WriteRect(string name, Rectangle rectangle)
		{
			WriteStartNamedElement("set", name);
			WriteElement("int", "left",   rectangle.Left.ToString());
			WriteElement("int", "top",    rectangle.Top.ToString());
			WriteElement("int", "right",  rectangle.Right.ToString());
			WriteElement("int", "bottom", rectangle.Bottom.ToString());
			WriteEndElement();
		}

		public void WriteStartNamedElement(string localName, string name)
		{
			WriteStartElement(localName);
			WriteAttributeString("name", name);
		}
	}
}
