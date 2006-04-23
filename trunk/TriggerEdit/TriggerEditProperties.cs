using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for TriggerEditProperties.
	/// </summary>
	public class TriggerEditProperties : System.Windows.Forms.Form
	{
		//----------
		// interface
		//----------

		#region

		public TriggerEditProperties()
		{
			InitializeComponent();
		}

		private void ok_btn__Click(object sender, System.EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		public float AnimationSpeed
		{
			get { return (float)animation_speed_udc_.Value; }
			set { animation_speed_udc_.Value = new decimal(value); }
		}

		public int MarkerLineCount
		{
			get { return (int)line_count_nud_.Value; }
			set { line_count_nud_.Value = new decimal(value); }
		}

		public float MarkerSpacing
		{
			get { return (float)spacing_udc_.Value; }
			set { spacing_udc_.Value = new decimal(value); }
		}

		public bool DisplayAction
		{
			get { return display_action_cb_.Checked; }
			set { display_action_cb_.Checked = value; }
		}

		/// <summary>
		/// Opacity of non-modal dialogs, in the range (0, 1].
		/// </summary>
		public float DialogOpacity
		{
			get { return (float)dialog_opacity_nud_.Value / 100.0f; }
			set { dialog_opacity_nud_.Value = new decimal(value * 100.0f); }
		}

		#endregion

		//----------
		// generated
		//----------

		#region code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.ok_btn_ = new System.Windows.Forms.Button();
			this.cancel_btn_ = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.animation_speed_udc_ = new System.Windows.Forms.NumericUpDown();
			this.spacing_udc_ = new System.Windows.Forms.NumericUpDown();
			this.line_count_nud_ = new System.Windows.Forms.NumericUpDown();
			this.label3 = new System.Windows.Forms.Label();
			this.dialog_opacity_nud_ = new System.Windows.Forms.NumericUpDown();
			this.label4 = new System.Windows.Forms.Label();
			this.label5 = new System.Windows.Forms.Label();
			this.display_action_cb_ = new System.Windows.Forms.CheckBox();
			((System.ComponentModel.ISupportInitialize)(this.animation_speed_udc_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.spacing_udc_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.line_count_nud_)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.dialog_opacity_nud_)).BeginInit();
			this.SuspendLayout();
			// 
			// ok_btn_
			// 
			this.ok_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_btn_.Location = new System.Drawing.Point(56, 168);
			this.ok_btn_.Name = "ok_btn_";
			this.ok_btn_.TabIndex = 0;
			this.ok_btn_.Text = "OK";
			this.ok_btn_.Click += new System.EventHandler(this.ok_btn__Click);
			// 
			// cancel_btn_
			// 
			this.cancel_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.cancel_btn_.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancel_btn_.Location = new System.Drawing.Point(144, 168);
			this.cancel_btn_.Name = "cancel_btn_";
			this.cancel_btn_.TabIndex = 1;
			this.cancel_btn_.Text = "Cancel";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(16, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(136, 16);
			this.label1.TabIndex = 2;
			this.label1.Text = "animation speed (pixels/s)";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(16, 40);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(136, 16);
			this.label2.TabIndex = 3;
			this.label2.Text = "marker spacing (pixels)";
			// 
			// animation_speed_udc_
			// 
			this.animation_speed_udc_.DecimalPlaces = 1;
			this.animation_speed_udc_.Location = new System.Drawing.Point(152, 16);
			this.animation_speed_udc_.Maximum = new System.Decimal(new int[] {
																									  1000,
																									  0,
																									  0,
																									  0});
			this.animation_speed_udc_.Name = "animation_speed_udc_";
			this.animation_speed_udc_.Size = new System.Drawing.Size(64, 20);
			this.animation_speed_udc_.TabIndex = 4;
			// 
			// spacing_udc_
			// 
			this.spacing_udc_.DecimalPlaces = 1;
			this.spacing_udc_.Location = new System.Drawing.Point(152, 40);
			this.spacing_udc_.Maximum = new System.Decimal(new int[] {
																							1000,
																							0,
																							0,
																							0});
			this.spacing_udc_.Name = "spacing_udc_";
			this.spacing_udc_.Size = new System.Drawing.Size(64, 20);
			this.spacing_udc_.TabIndex = 5;
			// 
			// line_count_nud_
			// 
			this.line_count_nud_.Location = new System.Drawing.Point(153, 64);
			this.line_count_nud_.Maximum = new System.Decimal(new int[] {
																								8,
																								0,
																								0,
																								0});
			this.line_count_nud_.Minimum = new System.Decimal(new int[] {
																								1,
																								0,
																								0,
																								0});
			this.line_count_nud_.Name = "line_count_nud_";
			this.line_count_nud_.Size = new System.Drawing.Size(64, 20);
			this.line_count_nud_.TabIndex = 7;
			this.line_count_nud_.Value = new System.Decimal(new int[] {
																							 1,
																							 0,
																							 0,
																							 0});
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(17, 64);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(136, 16);
			this.label3.TabIndex = 6;
			this.label3.Text = "marker line count";
			// 
			// dialog_opacity_nud_
			// 
			this.dialog_opacity_nud_.Location = new System.Drawing.Point(153, 112);
			this.dialog_opacity_nud_.Minimum = new System.Decimal(new int[] {
																									 1,
																									 0,
																									 0,
																									 0});
			this.dialog_opacity_nud_.Name = "dialog_opacity_nud_";
			this.dialog_opacity_nud_.Size = new System.Drawing.Size(64, 20);
			this.dialog_opacity_nud_.TabIndex = 9;
			this.dialog_opacity_nud_.Value = new System.Decimal(new int[] {
																								  1,
																								  0,
																								  0,
																								  0});
			// 
			// label4
			// 
			this.label4.Location = new System.Drawing.Point(17, 112);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(136, 16);
			this.label4.TabIndex = 8;
			this.label4.Text = "floating dialog opacity (%)";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(16, 88);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(136, 16);
			this.label5.TabIndex = 10;
			this.label5.Text = "display action on label";
			// 
			// display_action_cb_
			// 
			this.display_action_cb_.Location = new System.Drawing.Point(152, 88);
			this.display_action_cb_.Name = "display_action_cb_";
			this.display_action_cb_.Size = new System.Drawing.Size(16, 16);
			this.display_action_cb_.TabIndex = 11;
			// 
			// TriggerEditProperties
			// 
			this.AcceptButton = this.ok_btn_;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.cancel_btn_;
			this.ClientSize = new System.Drawing.Size(234, 199);
			this.Controls.Add(this.display_action_cb_);
			this.Controls.Add(this.label5);
			this.Controls.Add(this.dialog_opacity_nud_);
			this.Controls.Add(this.label4);
			this.Controls.Add(this.line_count_nud_);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.spacing_udc_);
			this.Controls.Add(this.animation_speed_udc_);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.cancel_btn_);
			this.Controls.Add(this.ok_btn_);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "TriggerEditProperties";
			this.ShowInTaskbar = false;
			this.Text = "Preferences";
			((System.ComponentModel.ISupportInitialize)(this.animation_speed_udc_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.spacing_udc_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.line_count_nud_)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.dialog_opacity_nud_)).EndInit();
			this.ResumeLayout(false);

		}
		
		
		#endregion

		#region data
		
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.Button ok_btn_;
		private System.Windows.Forms.Button cancel_btn_;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.NumericUpDown animation_speed_udc_;
		private System.Windows.Forms.NumericUpDown line_count_nud_;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.NumericUpDown dialog_opacity_nud_;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.CheckBox display_action_cb_;
		private System.Windows.Forms.NumericUpDown spacing_udc_;

		#endregion
	}
}
